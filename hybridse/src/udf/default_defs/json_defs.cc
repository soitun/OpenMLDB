/**
 * Copyright (c) 2023 OpenMLDB authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simdjson.h"
#include "udf/default_udf_library.h"
#include "udf/udf_library.h"
#include "udf/udf_registry.h"

namespace hybridse {
namespace udf {

void json_array_length(openmldb::base::StringRef* in, int32_t* sz, bool* is_null) {
    *is_null = true;

    simdjson::ondemand::parser parser;
    simdjson::padded_string json(in->data_, in->size_);
    simdjson::ondemand::document doc;
    auto err = parser.iterate(json).get(doc);
    if (err) {
        return;
    }
    simdjson::ondemand::array arr;
    err = doc.get_array().get(arr);
    if (err) {
        return;
    }
    size_t arr_sz;
    arr.count_elements().tie(arr_sz, err);
    if (err) {
        return;
    }

    *is_null = false;
    *sz = static_cast<int32_t>(arr_sz);
}

void DefaultUdfLibrary::InitJsonUdfs() {
    RegisterExternal("json_array_length")
    .args<openmldb::base::StringRef>(json_array_length)
    .doc(R"(
         @brief Returns the number of elements in the outermost JSON array.

         Null returned if input is not valid JSON array string.

         @param jsonArray JSON arry in string

         Example:

         @code{.sql}
           select json_array_length('[1, 2]')
           -- 2

           SELECT json_array_length('[1,2,3,{"f1":1,"f2":[5,6]},4]');
           -- 5

           select json_array_length('[1, 2')
           -- NULL
         @endcode

         @since 0.9.0)");
}

}  // namespace udf
}  // namespace hybridse