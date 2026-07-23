-------------------------------------------------------------------------------
--
-- MODULE      : glue_collab.lua
-- DESCRIPTION : Building glue for collaboration
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_collab",
        binding_object = "",
        initializer_name = "initialize_glue_collab",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "object_l3.hpp",
            "object_l5.hpp",
            "scheme.hpp",
            "glue_l5_extra.hpp",
            "../../../Plugins/Collab/loro_collab.hpp"
        },
        glues = {
            {
                -- 协作服务端地址（native 读 OS env MOGAN_LORO_SERVER；WASM 读
                -- window.MOGAN_LORO_SERVER / ?loro_server= 查询参数）。运行期可配。
                scm_name = "loro-collab-server-url",
                cpp_name = "loro_collab_server_url",
                ret_type = "string"
            },
            {
                -- 协作：以当前编辑器为 target 创建新云文档（连服务端、CREATE）
                scm_name = "loro-collab-create",
                cpp_name = "loro_collab_create",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                -- 协作：以当前编辑器为 target 加入已有云文档（JOIN <uuid>）
                scm_name = "loro-collab-join",
                cpp_name = "loro_collab_join",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "loro-collab-disconnect",
                cpp_name = "loro_collab_disconnect",
                ret_type = "void"
            },
            {
                scm_name = "loro-collab-active?",
                cpp_name = "loro_collab_is_active",
                ret_type = "bool"
            },
            {
                scm_name = "loro-collab-doc-id",
                cpp_name = "loro_collab_doc_id",
                ret_type = "string"
            },
            {
                -- 异步触发 HTTP 拉取服务端可用文档 UUID 列表（不建 WS、不阻塞 GUI）
                scm_name = "loro-collab-fetch-docs",
                cpp_name = "loro_collab_fetch_docs",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "loro-collab-docs-status",
                cpp_name = "loro_collab_docs_status",
                ret_type = "string"
            },
            {
                scm_name = "loro-collab-docs",
                cpp_name = "loro_collab_docs",
                ret_type = "array_string"
            },
            {
                scm_name = "loro-collab-poll",
                cpp_name = "loro_collab_poll",
                ret_type = "void",
            },
            {
                scm_name = "loro-enabled?",
                cpp_name = "loro_enabled",
                ret_type = "bool",
            }
        }
    }
end
