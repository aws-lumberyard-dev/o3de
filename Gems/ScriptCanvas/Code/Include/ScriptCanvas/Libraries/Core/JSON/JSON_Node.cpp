#include "JSON_Node.h"

namespace ScriptCanvas
{
    JSONView JSON_Nodeable::In([[maybe_unused]] JSONView sourceJSON)
    {
        m_jsonData = sourceJSON;

        // Iterate through the slots, call the respective function for the type using the slot name
        // as the key
        
      /*  JSON_NodeableNode* node = azrtti_cast<JSON_NodeableNode*>(GetNode());
        for (auto slot : node->GetAllSlots())
        {
            if (slot->IsConnected() && slot->IsData() && slot->IsOutput())
            {
                const AZStd::string& key = slot->GetName();
                
                auto dataType = slot->GetDataType();

                if (dataType == ScriptCanvas::Data::Type::String())
                {
                    m_jsonData.GetString(key);
                }
            }
        }*/

        return m_jsonData;
    }

    //void JSON_Nodeable::ConfigureSlots()
    //{
    //    ScriptCanvas::Node::ConfigureSlots();

    //    // In
    //    {
    //        ScriptCanvas::ExecutionSlotConfiguration slotConfiguration;
    //        slotConfiguration.m_name = "In";
    //        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
    //        AddSlot(slotConfiguration);
    //    }

    //    // Out
    //    {
    //        ScriptCanvas::ExecutionSlotConfiguration slotConfiguration;
    //        slotConfiguration.m_name = "Out";
    //        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
    //        AddSlot(slotConfiguration);
    //    }        
    //}

    //void JSON_Nodeable::ConfigureVisualExtensions()
    //{
    //    // The Slot Name provided by the user will be the KEY
    //    {
    //        ScriptCanvas::VisualExtensionSlotConfiguration visualExtensions(ScriptCanvas::VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);
    //        visualExtensions.m_name = "Specify Key & Type";
    //        visualExtensions.m_displayGroup = GetDataDisplayGroup();
    //        visualExtensions.m_identifier = GetAddNodelingOutputDataSlot();
    //        visualExtensions.m_connectionType = ScriptCVanvas::ConnectionTypes::Output;
    //        RegisterExtension(visualExtensions);
    //    }

    //    // The Slot Name provided by the user will be the KEY
    //    {
    //        ScriptCanvas::VisualExtensionSlotConfiguration visualExtensions(ScriptCanvas::VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);
    //        visualExtensions.m_name = "Key";
    //        visualExtensions.m_displayGroup = GetDataDisplayGroup();
    //        visualExtensions.m_identifier = AZ_CRC_CE("KeyStringProperty");
    //        visualExtensions.m_connectionType = ScriptCVanvas::ConnectionTypes::Input;

    //        RegisterExtension(visualExtensions);
    //    }
    //}

    //static ScriptCanvas::SlotConfiguration JSON_Nodeable::CreateDataSlot(AZStd::string_view name, AZStd::string dataDisplayGroup, ScriptCanvas::ConnectionType connectionType)
    //{
    //    ScriptCanvas::DynamicDataSlotConfiguration slotConfiguration;
    //    slotConfiguration.m_name = name;
    //    slotConfiguration.m_toolTipe = toolTip;
    //    slotConfiguration.SetConnectionType(connectionType);

    //    slotConfiguration.m_displayGroup = displayGroup;
    //    slotConfiguration.m_dynamicDataType = ScriptCnavas::DynamicDataType::Any;
    //    slotConfiguration.m_isUserAdded = true;

    //    slotConfiguration.m_addUniqueSlotByNameAndType = true;

    //    return slotConfiguration;
    //}

    //ScriptCanvas::SlotId JSON_Nodeable::HandleExtension(AZ::Crc32 extensionId)
    //{
    //    if (extensionId == AZ_CRC_CE("KeyStringProperty"))
    //    {
    //        ScriptCanvas::SlotId slotId = AddSlot(CreateDataSlot("key", "DataDisplayGroup", "", ScriptCanvas::ConnectionType::Output);
    //        
    //        // I need to add a parameter to the output exeuction
    //    }

    //    return ScriptCanvas::SlotId();
    //}
}
