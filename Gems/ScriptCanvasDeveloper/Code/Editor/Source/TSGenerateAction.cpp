/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>

#include <XMLDoc.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <Source/Translation/TranslationAsset.h>

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include "../Translation/TranslationHelper.h"

#pragma optimize("", off)


namespace ScriptCanvasDeveloperEditor
{
    namespace TranslationGenerator
    {
        void GenerateTranslationDatabase();

        QAction* TranslationDatabaseFileAction(QWidget* mainWindow)
        {
            QAction* qAction = nullptr;

            if (mainWindow)
            {
                qAction = new QAction(QAction::tr("Produce Localization Files for All Types"), mainWindow);
                qAction->setAutoRepeat(false);
                qAction->setToolTip("Produces a .names file for every reflected type supported by scripting.");
                qAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Alt+X", "Debug|Produce Localization Database")));
                mainWindow->addAction(qAction);
                mainWindow->connect(qAction, &QAction::triggered, &GenerateTranslationDatabase);
            }

            return qAction;
        }

        template <typename T>
        bool ShouldSkip(const T* object)
        {
            using namespace AZ::Script::Attributes;

            // Check for "ignore" attribute for ScriptCanvas
            auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<ExcludeFlags>*>(AZ::FindAttribute(ExcludeFrom, object->m_attributes));
            const bool excludeClass = excludeClassAttributeData && (static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr)) & static_cast<AZ::u64>(ExcludeFlags::List | ExcludeFlags::Documentation));

            if (excludeClass)
            {
                return true; // skip this class
            }

            return false;
        }

        void WriteString(rapidjson::Value& owner, const AZStd::string& key, const AZStd::string& value, rapidjson::Document& document)
        {
            if (key.empty() || value.empty())
            {
                return;
            }

            rapidjson::Value item(rapidjson::kStringType);
            item.SetString(value.c_str(), document.GetAllocator());

            rapidjson::Value keyVal(rapidjson::kStringType);
            keyVal.SetString(key.c_str(), document.GetAllocator());

            owner.AddMember(keyVal, item, document.GetAllocator());
        }

        void GetTypeNameAndDescription(AZ::TypeId typeId, AZStd::string& outName, AZStd::string& outDescription)
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Serialize Context is required");

            if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId))
            {
                if (classData->m_editData)
                {
                    outName = classData->m_editData->m_name ? classData->m_editData->m_name : "";
                    outDescription = classData->m_editData->m_description ? classData->m_editData->m_description : "";
                }
                else
                {
                    outName = classData->m_name;
                }
            }
        }


        AZStd::list<AZ::BehaviorEBus*> GatherCandidateEBuses(AZ::SerializeContext* /*serializeContext*/, AZ::BehaviorContext* behaviorContext)
        {
            AZStd::list<AZ::BehaviorEBus*> candidates;

            // We will skip buses that are ONLY registered on classes that derive from EditorComponentBase,
            // because they don't have a runtime implementation. Buses such as the TransformComponent which
            // is implemented by both an EditorComponentBase derived class and a Component derived class
            // will still appear
            AZStd::unordered_set<AZ::Crc32> skipBuses;
            AZStd::unordered_set<AZ::Crc32> potentialSkipBuses;
            AZStd::unordered_set<AZ::Crc32> nonSkipBuses;

            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                if (ShouldSkip(behaviorClass))
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        skipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }

                    continue;
                }

                auto baseClass = AZStd::find(behaviorClass->m_baseClasses.begin(),
                    behaviorClass->m_baseClasses.end(),
                    AzToolsFramework::Components::EditorComponentBase::TYPEINFO_Uuid());

                if (baseClass != behaviorClass->m_baseClasses.end())
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        potentialSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }
                }
                // If the Ebus does not inherit from EditorComponentBase then do not skip it
                else
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        nonSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }
                }
            }

            // Add buses which are not on the non-skip list to the skipBuses set
            for (auto potentialSkipBus : potentialSkipBuses)
            {
                if (nonSkipBuses.find(potentialSkipBus) == nonSkipBuses.end())
                {
                    skipBuses.insert(potentialSkipBus);
                }
            }

            for (const auto& ebusIter : behaviorContext->m_ebuses)
            {
                [[maybe_unused]] bool addContext = false;
                AZ::BehaviorEBus* ebus = ebusIter.second;

                if (ebus == nullptr)
                {
                    continue;
                }

                auto excludeEbusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebusIter.second->m_attributes));
                const bool excludeBus = excludeEbusAttributeData && static_cast<AZ::u64>(excludeEbusAttributeData->Get(nullptr))& static_cast<AZ::u64>(AZ::Script::Attributes::ExcludeFlags::Documentation);

                auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusIter.first.c_str()));
                if (!ebus || skipBusIterator != skipBuses.end() || excludeBus)
                {
                    continue;
                }

                candidates.push_back(ebus);
            }

            return candidates;
        }

        bool TranslatedEBusHandler([[maybe_unused]] AZ::BehaviorContext* behaviorContext, AZ::BehaviorEBus* ebus, TranslationFormat& translationRoot)
        {
            if (!ebus)
            {
                return false;
            }

            if (!ebus->m_createHandler || !ebus->m_destroyHandler)
            {
                return false;
            }

            AZ::BehaviorEBusHandler* handler(nullptr);
            if (ebus->m_createHandler->InvokeResult(handler))
            {
                Entry entry;

                // Generate the translation file
                entry.m_key = ebus->m_name;
                entry.m_context = "EBusHandler";
                entry.m_details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::Script::Attributes::Category);
                entry.m_details.m_tooltip = ebus->m_toolTip;
                entry.m_details.m_name = ebus->m_name;

                for (const AZ::BehaviorEBusHandler::BusForwarderEvent& event : handler->GetEvents())
                {
                    Method methodEntry;

                    AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(event.m_name);
                    methodEntry.m_key = cleanName;
                    methodEntry.m_details.m_category = "";
                    methodEntry.m_details.m_tooltip = "";
                    methodEntry.m_details.m_name = event.m_name;

                    // Arguments (Input Slots)
                    if (!event.m_parameters.empty())
                    {
                        for (size_t argIndex = AZ::eBehaviorBusForwarderEventIndices::ParameterFirst; argIndex < event.m_parameters.size(); ++argIndex)
                        {
                            const AZ::BehaviorParameter& parameter = event.m_parameters[argIndex];

                            Argument argument;

                            AZStd::string argumentKey = parameter.m_typeId.ToString<AZStd::string>();
                            AZStd::string argumentName = event.m_name;
                            AZStd::string argumentDescription = "";

                            if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > argIndex)
                            {
                                argumentName = event.m_metadataParameters[argIndex].m_name;
                                argumentDescription = event.m_metadataParameters[argIndex].m_toolTip;
                            }

                            if (argumentName.empty())
                            {
                                GetTypeNameAndDescription(parameter.m_typeId, argumentName, argumentDescription);
                            }

                            argument.m_typeId = argumentKey;
                            argument.m_details.m_name = argumentName;
                            argument.m_details.m_tooltip = argumentDescription;

                            methodEntry.m_arguments.push_back(argument);
                        }
                    }

                    auto resultIndex = AZ::eBehaviorBusForwarderEventIndices::Result;
                    const AZ::BehaviorParameter* resultParameter = event.HasResult() ? &event.m_parameters[resultIndex] : nullptr;
                    if (resultParameter)
                    {
                        Argument result;

                        AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                        AZStd::string resultName = event.m_name;
                        AZStd::string resultDescription = "";

                        if (!event.m_metadataParameters.empty() && event.m_metadataParameters.size() > resultIndex)
                        {
                            resultName = event.m_metadataParameters[resultIndex].m_name;
                            resultDescription = event.m_metadataParameters[resultIndex].m_toolTip;
                        }

                        if (resultName.empty())
                        {
                            GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);
                        }

                        result.m_typeId = resultKey;
                        result.m_details.m_name = resultName;
                        result.m_details.m_tooltip = resultDescription;

                        methodEntry.m_results.push_back(result);
                    }

                    entry.m_methods.push_back(methodEntry);

                }

                ebus->m_destroyHandler->Invoke(handler); // Destroys the Created EbusHandler

                translationRoot.m_entries.push_back(entry);
            }

            if (!translationRoot.m_entries.empty())
            {
                return true;
            }

            return false;

        }

        void SaveJSONData(const AZStd::string& filename, TranslationFormat& translationRoot)
        {
            rapidjson::Document document;
            document.SetObject();
            rapidjson::Value entries(rapidjson::kArrayType);

            // Here I'll need to parse translationRoot myself and produce the JSON
            for (const auto& entrySource : translationRoot.m_entries)
            {
                rapidjson::Value entry(rapidjson::kObjectType);
                rapidjson::Value value(rapidjson::kStringType);

                value.SetString(entrySource.m_key.c_str(), document.GetAllocator());
                entry.AddMember("key", value, document.GetAllocator());

                value.SetString(entrySource.m_context.c_str(), document.GetAllocator());
                entry.AddMember("context", value, document.GetAllocator());

                value.SetString(entrySource.m_variant.c_str(), document.GetAllocator());
                entry.AddMember("variant", value, document.GetAllocator());

                rapidjson::Value details(rapidjson::kObjectType);
                value.SetString(entrySource.m_details.m_name.c_str(), document.GetAllocator());
                details.AddMember("name", value, document.GetAllocator());

                WriteString(details, "category", entrySource.m_details.m_category, document);
                WriteString(details, "tooltip", entrySource.m_details.m_tooltip, document);

                entry.AddMember("details", details, document.GetAllocator());

                if (!entrySource.m_methods.empty())
                {
                    rapidjson::Value methods(rapidjson::kArrayType);

                    for (const auto& methodSource : entrySource.m_methods)
                    {
                        rapidjson::Value theMethod(rapidjson::kObjectType);

                        value.SetString(methodSource.m_key.c_str(), document.GetAllocator());
                        theMethod.AddMember("key", value, document.GetAllocator());

                        if (!methodSource.m_context.empty())
                        {
                            value.SetString(methodSource.m_context.c_str(), document.GetAllocator());
                            theMethod.AddMember("context", value, document.GetAllocator());
                        }

                        if (!methodSource.m_entry.m_name.empty())
                        {
                            rapidjson::Value entrySlot(rapidjson::kObjectType);
                            value.SetString(methodSource.m_entry.m_name.c_str(), document.GetAllocator());
                            entrySlot.AddMember("name", value, document.GetAllocator());

                            WriteString(entrySlot, "tooltip", methodSource.m_entry.m_tooltip, document);

                            theMethod.AddMember("entry", entrySlot, document.GetAllocator());
                        }

                        if (!methodSource.m_exit.m_name.empty())
                        {
                            rapidjson::Value exitSlot(rapidjson::kObjectType);
                            value.SetString(methodSource.m_exit.m_name.c_str(), document.GetAllocator());
                            exitSlot.AddMember("name", value, document.GetAllocator());

                            WriteString(exitSlot, "tooltip", methodSource.m_exit.m_tooltip, document);

                            theMethod.AddMember("exit", exitSlot, document.GetAllocator());
                        }

                        rapidjson::Value methodDetails(rapidjson::kObjectType);

                        value.SetString(methodSource.m_details.m_name.c_str(), document.GetAllocator());
                        methodDetails.AddMember("name", value, document.GetAllocator());

                        WriteString(methodDetails, "category", methodSource.m_details.m_category, document);
                        WriteString(methodDetails, "tooltip", methodSource.m_details.m_tooltip, document);

                        theMethod.AddMember("details", methodDetails, document.GetAllocator());

                        if (!methodSource.m_arguments.empty())
                        {
                            rapidjson::Value methodArguments(rapidjson::kArrayType);

                            [[maybe_unused]] size_t index = 0;
                            for (const auto& argSource : methodSource.m_arguments)
                            {
                                rapidjson::Value argument(rapidjson::kObjectType);
                                rapidjson::Value argumentDetails(rapidjson::kObjectType);

                                value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                                argument.AddMember("typeid", value, document.GetAllocator());

                                value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                                argumentDetails.AddMember("name", value, document.GetAllocator());

                                WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                                WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);


                                argument.AddMember("details", argumentDetails, document.GetAllocator());

                                methodArguments.PushBack(argument, document.GetAllocator());

                            }

                            theMethod.AddMember("params", methodArguments, document.GetAllocator());

                        }

                        if (!methodSource.m_results.empty())
                        {
                            rapidjson::Value methodArguments(rapidjson::kArrayType);

                            for (const auto& argSource : methodSource.m_results)
                            {
                                rapidjson::Value argument(rapidjson::kObjectType);
                                rapidjson::Value argumentDetails(rapidjson::kObjectType);

                                value.SetString(argSource.m_typeId.c_str(), document.GetAllocator());
                                argument.AddMember("typeid", value, document.GetAllocator());

                                value.SetString(argSource.m_details.m_name.c_str(), document.GetAllocator());
                                argumentDetails.AddMember("name", value, document.GetAllocator());

                                WriteString(argumentDetails, "category", argSource.m_details.m_category, document);
                                WriteString(argumentDetails, "tooltip", argSource.m_details.m_tooltip, document);

                                argument.AddMember("details", argumentDetails, document.GetAllocator());

                                methodArguments.PushBack(argument, document.GetAllocator());
                            }

                            
                            theMethod.AddMember("results", methodArguments, document.GetAllocator());

                        }

                        methods.PushBack(theMethod, document.GetAllocator());
                    }

                    entry.AddMember("methods", methods, document.GetAllocator());
                }

                entries.PushBack(entry, document.GetAllocator());
            }

            document.AddMember("entries", entries, document.GetAllocator());

            AZStd::string translationOutputFolder = AZStd::string::format("@devroot@/TranslationAssets");
            AZStd::string outputFileName = AZStd::string::format("%s/%s.names", translationOutputFolder.c_str(), filename.c_str());

            outputFileName = GraphCanvas::TranslationKey::Sanitize(outputFileName);

            AZStd::string endPath;
            AZ::StringFunc::Path::GetFolderPath(outputFileName.c_str(), endPath);

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(endPath.c_str()))
            {
                if (AZ::IO::FileIOBase::GetInstance()->CreatePath(endPath.c_str()) != AZ::IO::ResultCode::Success)
                {
                    AZ_Error("Translation", false, "Failed to create output folder");
                    return;
                }
            }


            char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(outputFileName.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
            endPath = resolvedBuffer;
            AZ::StringFunc::Path::Normalize(endPath);

            AZ::IO::SystemFile outputFile;
            if (!outputFile.Open(endPath.c_str(),
                AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("Translation", false, "Failed to open file for writing: %s", filename.c_str());
                return;
            }

            rapidjson::StringBuffer scratchBuffer;

            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);
            document.Accept(writer);

            outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
            outputFile.Close();

            scratchBuffer.Clear();
        }


        void TranslateNodes(AZ::SerializeContext* serializeContext, TranslationFormat& translationRoot)
        {
            VerificationSet verificationSet;
            GraphCanvas::TranslationKey translationKey;
            AZStd::vector<AZ::TypeId> nodes;

            auto getNodeClasses = [&serializeContext, &nodes](const AZ::SerializeContext::ClassData*, const AZ::Uuid& type)
            {

                bool foundBaseClass = false;
                auto baseClassVisitorFn = [&nodes, &type, &foundBaseClass](const AZ::SerializeContext::ClassData* reflectedBase, const AZ::TypeId& /*rttiBase*/)
                {
                    if (!reflectedBase)
                    {
                        foundBaseClass = false;
                        return false; // stop iterating
                    }

                    foundBaseClass = (reflectedBase->m_typeId == azrtti_typeid<ScriptCanvas::Node>());
                    if (foundBaseClass)
                    {
                        nodes.push_back(type);
                        return false; // we have a base, stop iterating
                    }

                    return true; // keep iterating
                };

                AZ::EntityUtils::EnumerateBaseRecursive(serializeContext, baseClassVisitorFn, type);

                return true;
            };

            serializeContext->EnumerateAll(getNodeClasses);

            for (auto& node : nodes)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(node);
                if (classData)
                {
                    AZStd::string keyName = classData->m_editData ? classData->m_editData->m_name : classData->m_name;
                    AZStd::string description = classData->m_editData ? classData->m_editData->m_description : "";

                    Entry entry;

                    AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(classData->m_name);

                    EntryDetails& details = entry.m_details;
                    entry.m_key = classData->m_typeId.ToString<AZStd::string>();
                    details.m_name = keyName;
                    details.m_tooltip = description;
                    entry.m_context = "Node";


                    // Tooltip attribute takes priority over the edit data description
                    AZStd::string tooltip = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::ToolTip);
                    if (!tooltip.empty())
                    {
                        details.m_tooltip = tooltip;
                    }

                    details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(classData, AZ::Script::Attributes::Category);
                    if (details.m_category.empty() && classData->m_editData)
                    {
                        auto elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        const AZStd::string categoryContext = GraphCanvasAttributeHelper::ReadStringAttribute(elementData->m_attributes, AZ::Script::Attributes::Category);
                        details.m_category = categoryContext;
                        if (!categoryContext.empty())
                        {
                            entry.m_context = categoryContext;
                        }
                    }

                    // Disambiguate if we end up with a duplicate key
                    translationKey << entry.m_context << entry.m_key << entry.m_variant;
                    auto verification = verificationSet.insert(translationKey);
                    if (!verification.second)
                    {
                        //entry.m_context = cleanName;
                    }

                    AZ::Entity* entity = aznew AZ::Entity(classData->m_name);
                    ScriptCanvas::Node* nodeComponent = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name));
                    entity->AddComponent(nodeComponent);
                    entity->Init();
                    

                    if (nodeComponent)
                    {
                        Method methodEntry;
                        methodEntry.m_key = nodeComponent->GetNodeName();
                        if (methodEntry.m_key.compare(entry.m_key) == 0)
                        {
                            methodEntry.m_key.append("_node");
                        }

                        methodEntry.m_details.m_name = nodeComponent->GetNodeName();
                        methodEntry.m_details.m_tooltip = (classData && classData->m_editData) ? classData->m_editData->m_description : nodeComponent->RTTI_GetTypeName();

                        auto allSlots = nodeComponent->GetAllSlots();
                        for (auto slot : allSlots)
                        {
                            if (slot->GetDescriptor().IsExecution() && slot->GetDescriptor().IsInput())
                            {
                                methodEntry.m_entry.m_name = slot->GetName();
                                methodEntry.m_entry.m_tooltip = slot->GetToolTip();
                            }
                            else if (slot->GetDescriptor().IsExecution() && slot->GetDescriptor().IsOutput())
                            {
                                methodEntry.m_exit.m_name = slot->GetName();
                                methodEntry.m_exit.m_tooltip = slot->GetToolTip();
                            }
                            else if (slot->GetDescriptor().IsData() && slot->GetDescriptor().IsInput())
                            {
                                Argument argument;

                                AZStd::string argumentKey = slot->GetName();
                                AZStd::string argumentName = slot->GetName();
                                AZStd::string argumentDescription = slot->GetToolTip();

                                argument.m_typeId = argumentKey;
                                argument.m_details.m_name = argumentName;
                                argument.m_details.m_category = "";
                                argument.m_details.m_tooltip = argumentDescription;

                                methodEntry.m_arguments.push_back(argument);
                            }
                            else if (slot->GetDescriptor().IsData() && slot->GetDescriptor().IsOutput())
                            {
                                Argument result;

                                AZStd::string resultKey = slot->GetName();
                                AZStd::string resultName = slot->GetName();
                                AZStd::string resultDescription = slot->GetToolTip();

                                result.m_typeId = resultKey;
                                result.m_details.m_name = resultName;
                                result.m_details.m_category = "";
                                result.m_details.m_tooltip = resultDescription;

                                methodEntry.m_results.push_back(result);
                            }
                        }

                        entry.m_methods.push_back(methodEntry);
                    }

                    if (entry.m_key.compare("NativeDatumNode") == 0 && entry.m_context.empty())
                    {
                        AZ_TracePrintf("Translation", "how?");
                    }
                    translationRoot.m_entries.push_back(entry);

                    translationKey.clear();
                }
            }
        }

        void TranslateOnDemandReflectedTypes([[maybe_unused]] AZ::SerializeContext* serializeContext, AZ::BehaviorContext* behaviorContext, TranslationFormat& translationRoot)
        {
            AZStd::vector<AZ::Uuid> onDemandReflectedTypes;

            for (auto& typePair : behaviorContext->m_typeToClassMap)
            {
                if (behaviorContext->IsOnDemandTypeReflected(typePair.first))
                {
                    onDemandReflectedTypes.push_back(typePair.first);
                }

                // Check for methods that come from node generics
                if (typePair.second->HasAttribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric))
                {
                    onDemandReflectedTypes.push_back(typePair.first);
                }
            }

            // Now that I know all the on demand reflected, I'll dump it out
            for (auto& onDemandReflectedType : onDemandReflectedTypes)
            {
                AZ::BehaviorClass* behaviorClass = behaviorContext->m_typeToClassMap[onDemandReflectedType];
                if (behaviorClass)
                {
                    Entry entry;

                    EntryDetails& details = entry.m_details;
                    details.m_name = behaviorClass->m_name;

                    // Get the pretty name
                    AZStd::string prettyName;
                    if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
                    {
                        AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(prettyName, *behaviorContext);
                    }

                    entry.m_context = "";
                    entry.m_key = behaviorClass->m_name;

                    if (!prettyName.empty())
                    {
                        entry.m_key = prettyName;
                        details.m_name = prettyName;
                    }

                    details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                    details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);

                    for (auto& methodPair : behaviorClass->m_methods)
                    {
                        AZ::BehaviorMethod* behaviorMethod = methodPair.second;
                        if (behaviorMethod)
                        {
                            Method methodEntry;

                            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                            methodEntry.m_key = cleanName;
                            methodEntry.m_context = entry.m_key;

                            methodEntry.m_details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                            methodEntry.m_details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorMethod, AZ::Script::Attributes::ToolTip);
                            methodEntry.m_details.m_name = methodPair.second->m_name;

                            // Strip the className from the methodName
                            AZStd::string qualifiedName = behaviorClass->m_name + "::";
                            AzFramework::StringFunc::Replace(methodEntry.m_details.m_name, qualifiedName.c_str(), "");

                            AZStd::string cleanMethodName = methodEntry.m_details.m_name;

                            methodEntry.m_entry.m_name = "In";
                            methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
                            methodEntry.m_exit.m_name = "Out";
                            methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

                            // Arguments (Input Slots)
                            if (behaviorMethod->GetNumArguments() > 0)
                            {
                                for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                                {
                                    const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                                    Argument argument;

                                    AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                                    AZStd::string argumentName = parameter->m_name;
                                    AZStd::string argumentDescription = "";

                                    GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                                    argument.m_typeId = argumentKey;
                                    argument.m_details.m_name = argumentName;
                                    argument.m_details.m_category = "";
                                    argument.m_details.m_tooltip = argumentDescription;

                                    methodEntry.m_arguments.push_back(argument);
                                }
                            }

                            const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                            if (resultParameter)
                            {
                                Argument result;

                                AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                                AZStd::string resultName = resultParameter->m_name;
                                AZStd::string resultDescription = "";

                                GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                                result.m_typeId = resultKey;
                                result.m_details.m_name = resultName;
                                result.m_details.m_tooltip = resultDescription;

                                methodEntry.m_results.push_back(result);
                            }

                            entry.m_methods.push_back(methodEntry);
                        }
                    }

                    translationRoot.m_entries.push_back(entry);
                }

            }
        }

        void TranslateEBus(AZ::SerializeContext* serializeContext, AZ::BehaviorContext* behaviorContext)
        {
            AZStd::list<AZ::BehaviorEBus*> ebuses = GatherCandidateEBuses(serializeContext, behaviorContext);

            // Get the request buses
            for (auto ebus : ebuses)
            {
                if (ShouldSkip(ebus))
                {
                    continue;
                }

                TranslationFormat translationRoot;

                // Get the handlers
                if (!TranslatedEBusHandler(behaviorContext, ebus, translationRoot))
                {
                    if (ebus->m_events.empty())
                    {
                        // Skip empty ebuses
                        continue;
                    }

                    Entry entry;

                     // Generate the translation file
                    entry.m_key = ebus->m_name;
                    entry.m_details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::Script::Attributes::Category);;
                    entry.m_details.m_tooltip = ebus->m_toolTip;
                    entry.m_details.m_name = ebus->m_name;

                    const AZStd::string prettyName = GraphCanvasAttributeHelper::GetStringAttribute(ebus, AZ::ScriptCanvasAttributes::PrettyName);
                    if (!prettyName.empty())
                    {
                        entry.m_details.m_name = prettyName;
                    }

                    for (auto event : ebus->m_events)
                    {
                        const AZ::BehaviorEBusEventSender& ebusSender = event.second;

                        // TODO: Figure out if I need to make a distinction here
                        AZ::BehaviorMethod* method = ebusSender.m_event;
                        if (!method)
                        {
                            method = ebusSender.m_broadcast;
                        }

                        if (!method)
                        {
                            continue;
                        }

                        Method eventEntry;
                        const char* eventName = event.first.c_str();

                        AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(eventName);
                        eventEntry.m_key = cleanName;

                        eventEntry.m_details.m_name = cleanName;
                        eventEntry.m_details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(&ebusSender, AZ::Script::Attributes::ToolTip);

                        eventEntry.m_entry.m_name = "In";
                        eventEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", eventEntry.m_details.m_name.c_str());
                        eventEntry.m_exit.m_name = "Out";
                        eventEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", eventEntry.m_details.m_name.c_str());

                        size_t start = method->HasBusId() ? 1 : 0;
                        for (size_t i = start; i < method->GetNumArguments(); ++i)
                        {
                            Argument argument;
                            auto argumentType = method->GetArgument(i)->m_typeId;

                            argument.m_typeId = argumentType.ToString<AZStd::string>();
                            argument.m_details.m_tooltip = *method->GetArgumentToolTip(i);
                            argument.m_details.m_name = method->GetArgument(i)->m_name;

                            GetTypeNameAndDescription(argumentType, argument.m_details.m_name, argument.m_details.m_tooltip);

                            eventEntry.m_arguments.push_back(argument);
                        }

                        if (method->HasResult())
                        {
                            Argument result;

                            auto resultType = method->GetResult()->m_typeId;
                            result.m_typeId = resultType.ToString<AZStd::string>();
                            result.m_details.m_name = *method->GetResult()->m_name;

                            GetTypeNameAndDescription(resultType, result.m_details.m_name, result.m_details.m_tooltip);

                            eventEntry.m_results.push_back(result);
                        }

                        entry.m_methods.push_back(eventEntry);

                    }

                    translationRoot.m_entries.push_back(entry);

                    SaveJSONData(AZStd::string::format("EBus/Senders/%s", ebus->m_name.c_str()), translationRoot);
                }
                else
                {
                    SaveJSONData(AZStd::string::format("EBus/Handlers/%s", ebus->m_name.c_str()), translationRoot);
                }
            }
        }

        void TranslateBehaviorClasses(AZ::SerializeContext* /*serializeContext*/, AZ::BehaviorContext* behaviorContext)
        {
            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                if (ShouldSkip(behaviorClass))  
                {
                    continue;
                }

                AZStd::string className = behaviorClass->m_name;
                AZStd::string prettyName = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::ScriptCanvasAttributes::PrettyName);
                if (!prettyName.empty())
                {
                    className = prettyName;
                }

                {
                    TranslationFormat translationRoot;

                    Entry entry;

                    EntryDetails& details = entry.m_details;
                    details.m_name = className;
                    details.m_category = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::Category);
                    details.m_tooltip = GraphCanvasAttributeHelper::GetStringAttribute(behaviorClass, AZ::Script::Attributes::ToolTip);
                    entry.m_context = "";
                    entry.m_key = behaviorClass->m_name;

                    if (!behaviorClass->m_methods.empty())
                    {
                        for (const auto& methodPair : behaviorClass->m_methods)
                        {
                            const AZ::BehaviorMethod* behaviorMethod = methodPair.second;

                            Method methodEntry;

                            AZStd::string cleanName = GraphCanvas::TranslationKey::Sanitize(methodPair.first);

                            methodEntry.m_key = cleanName;
                            methodEntry.m_context = className;

                            methodEntry.m_details.m_category = "";
                            methodEntry.m_details.m_tooltip = "";
                            methodEntry.m_details.m_name = methodPair.second->m_name;

                            methodEntry.m_entry.m_name = "In";
                            methodEntry.m_entry.m_tooltip = AZStd::string::format("When signaled, this will invoke %s", methodEntry.m_details.m_name.c_str());
                            methodEntry.m_exit.m_name = "Out";
                            methodEntry.m_exit.m_tooltip = AZStd::string::format("Signaled after %s is invoked", methodEntry.m_details.m_name.c_str());

                            // Arguments (Input Slots)
                            if (behaviorMethod->GetNumArguments() > 0)
                            {
                                for (size_t argIndex = 0; argIndex < behaviorMethod->GetNumArguments(); ++argIndex)
                                {
                                    const AZ::BehaviorParameter* parameter = behaviorMethod->GetArgument(argIndex);

                                    Argument argument;

                                    AZStd::string argumentKey = parameter->m_typeId.ToString<AZStd::string>();
                                    AZStd::string argumentName = parameter->m_name;
                                    AZStd::string argumentDescription = "";

                                    GetTypeNameAndDescription(parameter->m_typeId, argumentName, argumentDescription);

                                    argument.m_typeId = argumentKey;
                                    argument.m_details.m_name = argumentName;
                                    argument.m_details.m_category = "";
                                    argument.m_details.m_tooltip = argumentDescription;

                                    methodEntry.m_arguments.push_back(argument);
                                }
                            }

                            const AZ::BehaviorParameter* resultParameter = behaviorMethod->HasResult() ? behaviorMethod->GetResult() : nullptr;
                            if (resultParameter)
                            {
                                Argument result;

                                AZStd::string resultKey = resultParameter->m_typeId.ToString<AZStd::string>();

                                AZStd::string resultName = resultParameter->m_name;
                                AZStd::string resultDescription = "";

                                GetTypeNameAndDescription(resultParameter->m_typeId, resultName, resultDescription);

                                result.m_typeId = resultKey;
                                result.m_details.m_name = resultName;
                                result.m_details.m_tooltip = resultDescription;

                                methodEntry.m_results.push_back(result);
                            }

                            entry.m_methods.push_back(methodEntry);
                        }
                    }

                    translationRoot.m_entries.push_back(entry);

                    AZStd::string fileName = AZStd::string::format("Classes/%s", className.c_str());

                    SaveJSONData(fileName, translationRoot);
                }
            }
        }

        void GenerateTranslationDatabase()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AZ::BehaviorContext* behaviorContext{};
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

            AZ_Assert(serializeContext && behaviorContext, "Must have valid Serialization and Behavior Contexts");

            // BehaviorClass
            {
                TranslateBehaviorClasses(serializeContext, behaviorContext);
            }

            // On Demand Reflected Types
            {
                TranslationFormat onDemandTranslationRoot;
                TranslateOnDemandReflectedTypes(serializeContext, behaviorContext, onDemandTranslationRoot);
                SaveJSONData("Types/OnDemandReflectedTypes", onDemandTranslationRoot);
            }

            // Native nodes
            {
                TranslationFormat nodeTranslationRoot;
                TranslateNodes(serializeContext, nodeTranslationRoot);
                SaveJSONData("Nodes/ScriptCanvasNodes", nodeTranslationRoot);
            }

            // EBus
            {
                TranslateEBus(serializeContext, behaviorContext);
            }
        }
    }
}
