/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "StdAfx.h"
#include "LmbrCentralEditor.h"
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorCylinderShapeComponent.h"
#include <AzToolsFramework/Application/ToolsApplication.h>

namespace LmbrCentral
{
    // Serialized legacy EditorCylinderShapeComponent v1.
    const char kEditorCylinderComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorCylinderShapeComponent" field="element" version="1" type="{D5FC4745-3C75-47D9-8C10-9F89502487DE}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="2283148451428660584" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="CylinderShapeConfig" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
                <Class name="float" field="Height" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                <Class name="float" field="Radius" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorCylinderShapeComponentFromVersion1
        : public LoadReflectedObjectTest<AZ::ComponentApplication, LmbrCentralEditorModule, EditorCylinderShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorCylinderComponentVersion1; }

        void SetUp() override
        {
            LoadReflectedObjectTest::SetUp();

            if (m_object)
            {
                m_editorCylinderShapeComponent = m_object.get();
                m_CylinderShapeConfig = m_editorCylinderShapeComponent->GetConfiguration();
            }
        }

        EditorCylinderShapeComponent* m_editorCylinderShapeComponent = nullptr;
        CylinderShapeConfig m_CylinderShapeConfig;

    };

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_NE(m_editorCylinderShapeComponent, nullptr);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        EXPECT_FLOAT_EQ(m_CylinderShapeConfig.m_height, 1.57f);
    }

    TEST_F(LoadEditorCylinderShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        EXPECT_FLOAT_EQ(m_CylinderShapeConfig.m_radius, 0.57f);
    }
}

