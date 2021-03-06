#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import service
import survey_common
import survey_utils
import validation_utils
import validation_common
import errors

@service.api
def put(request, survey_id, rename_survey_input):
    validation_common.validate_survey_id(survey_id)

    survey_name = rename_survey_input.get('survey_name')
    validation_utils.validate_param(survey_name, 'survey_name', validation_utils.is_not_blank_str)

    survey_common.ensure_survey_exists(survey_id)

    survey_utils.get_survey_table().update_item(
        Key={'survey_id':survey_id},
        UpdateExpression='SET survey_name = :survey_name',
        ExpressionAttributeValues={':survey_name':survey_name}
    )

    return "success"
