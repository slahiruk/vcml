/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#include "vcml/properties/property_base.h"

namespace vcml {

    property_base::property_base(const char* nm, sc_module* parent):
        sc_attr_base(nm),
        m_name(),
        m_parent(parent) {
        if (m_parent == NULL) {
            sc_simcontext* simc = sc_core::sc_get_curr_simcontext();
            m_parent = simc->hierarchy_curr();
        }

        VCML_ERROR_ON(!m_parent, "property '%s' declared outside module", nm);

        m_parent->add_attribute(*this);
        m_name = string(m_parent->name()) + SC_HIERARCHY_CHAR + nm;
    }

    property_base::~property_base() {
        m_parent->remove_attribute(name());
    }

}
