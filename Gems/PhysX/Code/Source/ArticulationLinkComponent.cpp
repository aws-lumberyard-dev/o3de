/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ArticulationLinkComponent.h>

#include <AzFramework/Physics/SystemBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    // Definitions are put in .cpp so we can have AZStd::unique_ptr<T> member with forward declared T in the header
    // This causes AZStd::unique_ptr<T> ctor/dtor to be generated when full type info is available
    ArticulationLinkComponent::ArticulationLinkComponent()
    {
        InitPhysicsTickHandler();
    }

    ArticulationLinkComponent::~ArticulationLinkComponent() = default;

    ArticulationLinkComponent::ArticulationLinkComponent(ArticulationLinkConfiguration& config)
        : m_config(config)
    {
        InitPhysicsTickHandler();
    }

    void ArticulationLinkData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkData>()
                ->Version(1)
                ->Field("LinkConfiguration", &ArticulationLinkData::m_articulationLinkConfiguration)
                ->Field("ShapeColliderPair", &ArticulationLinkData::m_shapeColliderConfiguration)
                ->Field("RelativeTransform", &ArticulationLinkData::m_relativeTransform)
                ->Field("ChildLinks", &ArticulationLinkData::m_childLinks)
            ;
        }
    }

    void ArticulationLinkComponent::Reflect(AZ::ReflectContext* context)
    {
        ArticulationLinkData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArticulationLinkComponent, AZ::Component>()
                ->Version(1)
                ->Field("ArticulationLinkData", &ArticulationLinkComponent::m_articulationLinkData)
                ->Field("ArticulationLinkConfiguration", &ArticulationLinkComponent::m_config);
        }
    }

    void ArticulationLinkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void ArticulationLinkComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void ArticulationLinkComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void ArticulationLinkComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    bool ArticulationLinkComponent::IsRootArticulation() const
    {
        return IsRootArticulationEntity<ArticulationLinkComponent>(GetEntity());
    }

    void ArticulationLinkComponent::Activate()
    {
        if (IsRootArticulation())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RegisterSceneSimulationFinishHandler(m_attachedSceneHandle, m_sceneFinishSimHandler);
            }

            CreateArticulation();
        }
    }

    void ArticulationLinkComponent::Deactivate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        if (m_articulation)
        {
            DestroyArticulation();
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void ArticulationLinkComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
    }

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    void ArticulationLinkComponent::CreateArticulation()
    {
        AzPhysics::SceneInterface* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (!sceneInterface)
        {
            return;
        }

        physx::PxPhysics* pxPhysics = GetPhysXSystem()->GetPxPhysics();
        m_articulation = pxPhysics->createArticulationReducedCoordinate();


        const auto& rootLinkConfiguration = m_articulationLinkData->m_articulationLinkConfiguration;
        SetRootSpecificProperties(rootLinkConfiguration);

        CreateChildArticulationLinks(nullptr, *m_articulationLinkData.get());

        // Add articulation to the scene
        AzPhysics::Scene* scene = sceneInterface->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_WRITE_LOCK(pxScene);
        pxScene->addArticulation(*m_articulation);
    }

    void ArticulationLinkComponent::SetRootSpecificProperties(const ArticulationLinkConfiguration& rootLinkConfiguration)
    {
        m_articulation->setSleepThreshold(rootLinkConfiguration.m_sleepMinEnergy);
        if (rootLinkConfiguration.m_startAsleep)
        {
            m_articulation->putToSleep();
        }

        physx::PxArticulationFlags articulationFlags(0);
        if (rootLinkConfiguration.m_isFixedBase)
        {
            articulationFlags.raise(physx::PxArticulationFlag::eFIX_BASE);
        }

        if (!rootLinkConfiguration.m_selfCollide)
        {
            // Disable collisions between the articulation's links (note that parent/child collisions
            // are disabled internally in either case).
            articulationFlags.raise(physx::PxArticulationFlag::eDISABLE_SELF_COLLISION);
        }

        // TODO: Expose these in the configuration
        //      eDRIVE_LIMITS_ARE_FORCES //!< Limits for drive effort are forces and torques rather than impulses
        //      eCOMPUTE_JOINT_FORCES //!< Enable in order to be able to query joint solver .
    }

    void ArticulationLinkComponent::CreateChildArticulationLinks(
        physx::PxArticulationLink* parentLink, const ArticulationLinkData& thisLinkData)
    {
        physx::PxTransform thisLinkTransform;
        if (parentLink)
        {
            physx::PxTransform parentLinkTransform = parentLink->getGlobalPose();
            physx::PxTransform thisLinkRelativeTransform = PxMathConvert(thisLinkData.m_relativeTransform);
            thisLinkTransform = parentLinkTransform * thisLinkRelativeTransform;
        }
        else
        {
            thisLinkTransform = PxMathConvert(GetEntity()->GetTransform()->GetWorldTM());
        }

        physx::PxArticulationLink* thisPxLink = m_articulation->createLink(parentLink, thisLinkTransform);
        if (!thisPxLink)
        {
            AZ_Error("PhysX", false, "Failed to create articulation link at root %s", GetEntity()->GetName().c_str());
            return;
        }

        AzPhysics::SimulatedBodyHandle articulationLinkHandle =
            AZ::Interface<AzPhysics::SceneInterface>::Get()->AddSimulatedBody(m_attachedSceneHandle, &thisLinkData.m_articulationLinkConfiguration);
        if (articulationLinkHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            AZ_Error("PhysX", false, "Failed to create a simulated body for the articulation link at root %s",
                GetEntity()->GetName().c_str());
            return;
        }

        m_articulationLinks.emplace_back(articulationLinkHandle);

        AzPhysics::SimulatedBody* simulatedBody =
            AZ::Interface<AzPhysics::SceneInterface>::Get()->GetSimulatedBodyFromHandle(m_attachedSceneHandle, articulationLinkHandle);

        ArticulationLink* articulationLink = azrtti_cast<ArticulationLink*>(simulatedBody);
        articulationLink->SetPxArticulationLink(thisPxLink);
        articulationLink->SetupFromLinkData(thisLinkData);

        if (parentLink)
        {
            physx::PxArticulationJointReducedCoordinate* inboundJoint =
                thisPxLink->getInboundJoint()->is<physx::PxArticulationJointReducedCoordinate>();
            // TODO: Set the values for joints from thisLinkData
            inboundJoint->setJointType(physx::PxArticulationJointType::eFIX);
            inboundJoint->setParentPose(PxMathConvert(thisLinkData.m_relativeTransform));
            inboundJoint->setChildPose(physx::PxTransform(physx::PxIdentity));
        }

        for (const auto& childLink : thisLinkData.m_childLinks)
        {
            CreateChildArticulationLinks(thisPxLink, *childLink);
        }
    }


    void ArticulationLinkComponent::DestroyArticulation()
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        scene->RemoveSimulatedBodies(m_articulationLinks);
        m_articulationLinks.clear();

        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
        PHYSX_SCENE_WRITE_LOCK(pxScene);
        m_articulation->release();
    }

    void ArticulationLinkComponent::InitPhysicsTickHandler()
    {
        m_sceneFinishSimHandler = AzPhysics::SceneEvents::OnSceneSimulationFinishHandler(
            [this]([[maybe_unused]] AzPhysics::SceneHandle sceneHandle, float fixedDeltatime)
            {
                PostPhysicsTick(fixedDeltatime);
            },
            aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));
    }
    
    void ArticulationLinkComponent::PostPhysicsTick([[maybe_unused]] float fixedDeltaTime)
    {
        AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SceneInterface>::Get()->GetScene(m_attachedSceneHandle);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxScene);

        if (m_articulation->isSleeping())
        {
            return;
        }

        physx::PxArticulationLink* links[MaxArticulationLinks] = { 0 };
        m_articulation->getLinks(links, MaxArticulationLinks);

        const physx::PxU32 linksNum = m_articulation->getNbLinks();
        AZ_Assert(
            linksNum <= MaxArticulationLinks,
            "Error. Number of articulation links %d is greater than the maximum supported %d",
            linksNum,
            MaxArticulationLinks);

        for (physx::PxU32 linkIndex = 0; linkIndex < linksNum; ++linkIndex)
        {
            physx::PxArticulationLink* link = links[linkIndex];
            physx::PxTransform pxGlobalPose = link->getGlobalPose();
            AZ::Transform globalTransform = PxMathConvert(pxGlobalPose);
            ActorData* linkActorData = Utils::GetUserData(link);
            if (linkActorData)
            {
                AZ::EntityId linkEntityId = linkActorData->GetEntityId();
                AZ::TransformBus::Event(linkEntityId, &AZ::TransformBus::Events::SetWorldTM, globalTransform);
            }
        }
    }

    const ArticulationLinkConfiguration& ArticulationLinkComponent::GetConfiguration() const
    {
        return m_config;
    }

#else
    void ArticulationLinkComponent::CreateArticulation(){}
    void ArticulationLinkComponent::CreateChildArticulationLinks(physx::PxArticulationLink*, const ArticulationLinkData&){}
    void ArticulationLinkComponent::DestroyArticulation(){}
    void ArticulationLinkComponent::InitPhysicsTickHandler(){}
    void ArticulationLinkComponent::PostPhysicsTick(float){}
#endif

} // namespace PhysX
