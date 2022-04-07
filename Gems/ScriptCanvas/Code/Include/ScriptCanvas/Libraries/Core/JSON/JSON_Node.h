#pragma once

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

#include <Include/ScriptCanvas/Libraries/Core/JSON/JSON_Node.generated.h>

#include <ScriptCanvas/Core/Node.h>

#include <Libraries/Core/JSON/JSON.h>

namespace ScriptCanvas
{
    class JSON_Nodeable : public ScriptCanvas::Nodeable
    {
    public:
        SCRIPTCANVAS_NODE(JSON_Nodeable);

        ScriptCanvas::TypedNodePropertyInterface<ScriptCanvas::Data::StringType> m_keyInterface;
    };

/**
 * 
 
class JSON_Nodeable : public ScriptCanvas::Nodes::NodeableNode
                    , public ScriptCanvas::NodePropertyInterfaceListener
{
public:

    AZ_COMPONENT(JSON_Nodeable, "{}", SCriptCanvas::Nodes::NodeableNode);

    JSON_Nodeable()
    {
        SetNodeable(AZSTd::make_unique<JSONNode>());
    }

    void ConfigureSlots() override;

    void ConfigureVisualExtensions() override;
    ScriptCanvas::SlotId HandleExtension(AZ::Crc32 extensionId) override;

    AZStd::unordered_str<AZ::Uuid> GetSupportedSlotTypes() const override
    {
        AZStd::unordered_set<AZ::Uuid> types;
        types.insert(azrtti_typeid<ScriptCanvas::Data::NumberType>());
        types.insert(azrtti_typeid<ScriptCanvas::Data::StringType>());
        types.insert(azrtti_typeid<bool>());
        types.insert(azrtti_typeid<JSONView::TYPEINFO_Uuid()>());
        types.insert(azrtti_typeid<AZStd::vector<JSONView>>());

        return types;
    }

    void OnInit() override
    {
        m_keyInterface.SetPRopertyReference(&m_key);
        m_keyInterface.RegisterListener(this);
    }

    static void Reflect(AZ::ReflectContext* context)
    {
        using namespace ScriptCanvas;

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JSON_Nodeable, ScriptCanvas::Node::NodeableNode>()
            ->Field("m_key", &JSON_Node::m_key)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<JSON_Nodeable>("JSON_Helper", "")
                ->ClassElement(AZ::Edito::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "JSON")
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class("JSON_Helper");
        }
    }

private:

    static constexpr const char* GetDataDisplayGroup() { return "DataDisplayGroup"; }
    static constexpr AZ::Crc32 GetAddNodelingOutputDataSlot() { return AZ_CRC_CE("AddNodelingOutpuitDataSlot"); }

    ScriptCanvas::SlotId CreateDataSlot(AZStd::string_view name, AZStd::string_view toolTip, ScriptCanvas::ConnectionType connectionType);

    //Slots can be the following types:
    // NujmberType (GetDobule, GetInteger)
    // bool (GetBool)
    // AZStd::string (GetString)
    // AZStd::vector<JSONView> (GetArray)
    // JSONView (GetObject)

    AZStd::string m_key;

    ScriptCanvas::Data::StringType GetKey() const { return *m_keyInterface.GetPropertyData(); }
    
};

  
 */

}
