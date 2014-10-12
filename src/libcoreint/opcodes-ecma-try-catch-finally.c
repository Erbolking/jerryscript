/* Copyright 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "globals.h"
#include "interpreter.h"
#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * 'Try' opcode handler.
 *
 * See also: ECMA-262 v5, 12.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_try (opcode_t opdata, /**< operation data */
            int_data_t *int_data) /**< interpreter context */
{
  const idx_t block_end_oc_idx_1 = opdata.data.try.oc_idx_1;
  const idx_t block_end_oc_idx_2 = opdata.data.try.oc_idx_2;
  const opcode_counter_t try_end_oc = (opcode_counter_t) (
    calc_opcode_counter_from_idx_idx (block_end_oc_idx_1, block_end_oc_idx_2) + int_data->pos);

  int_data->pos++;

  ecma_completion_value_t try_completion = run_int_loop (int_data);
  JERRY_ASSERT ((!ecma_is_completion_value_empty (try_completion) && int_data->pos < try_end_oc)
                || (ecma_is_completion_value_empty (try_completion) && int_data->pos == try_end_oc));
  int_data->pos = try_end_oc;

  opcode_t next_opcode = read_opcode (int_data->pos);
  JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);

  if (ecma_is_completion_value_exit (try_completion))
  {
    return try_completion;
  }

  if (next_opcode.data.meta.type == OPCODE_META_TYPE_CATCH)
  {
    const opcode_counter_t catch_end_oc = (opcode_counter_t) (
      read_meta_opcode_counter (OPCODE_META_TYPE_CATCH, int_data) + int_data->pos);
    int_data->pos++;

    if (ecma_is_completion_value_throw (try_completion))
    {
      next_opcode = read_opcode (int_data->pos++);
      JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);
      JERRY_ASSERT (next_opcode.data.meta.type == OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER);

      const idx_t catch_exc_val_var_name_lit_idx = next_opcode.data.meta.data_1;
      ecma_string_t *catch_exc_var_name_str_p = ecma_new_ecma_string_from_lit_index (catch_exc_val_var_name_lit_idx);

      ecma_object_t *old_env_p = int_data->lex_env_p;
      ecma_object_t *catch_env_p = ecma_create_decl_lex_env (old_env_p);
      ecma_completion_value_t completion = ecma_op_create_mutable_binding (catch_env_p,
                                                                           catch_exc_var_name_str_p,
                                                                           false);
      JERRY_ASSERT (ecma_is_completion_value_empty (completion));
      completion = ecma_op_set_mutable_binding (catch_env_p,
                                                catch_exc_var_name_str_p,
                                                try_completion.u.value,
                                                false);
      JERRY_ASSERT (ecma_is_completion_value_empty (completion));

      ecma_deref_ecma_string (catch_exc_var_name_str_p);

      int_data->lex_env_p = catch_env_p;

      ecma_free_completion_value (try_completion);
      try_completion = run_int_loop (int_data);

      int_data->lex_env_p = old_env_p;

      ecma_deref_object (catch_env_p);
      
      JERRY_ASSERT ((!ecma_is_completion_value_empty (try_completion) && int_data->pos < catch_end_oc)
                    || (ecma_is_completion_value_empty (try_completion) && int_data->pos == catch_end_oc));
    }

    int_data->pos = catch_end_oc;
  }

  next_opcode = read_opcode (int_data->pos);
  JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);

  if (ecma_is_completion_value_exit (try_completion))
  {
    return try_completion;
  }

  if (next_opcode.data.meta.type == OPCODE_META_TYPE_FINALLY)
  {
    const opcode_counter_t finally_end_oc = (opcode_counter_t) (
      read_meta_opcode_counter (OPCODE_META_TYPE_FINALLY, int_data) + int_data->pos);
    int_data->pos++;

    ecma_completion_value_t finally_completion = run_int_loop (int_data);
    JERRY_ASSERT ((!ecma_is_completion_value_empty (finally_completion) && int_data->pos < finally_end_oc)
                  || (ecma_is_completion_value_empty (finally_completion) && int_data->pos == finally_end_oc));
    int_data->pos = finally_end_oc;

    if (!ecma_is_completion_value_empty (finally_completion))
    {
      ecma_free_completion_value (try_completion);
      try_completion = finally_completion;
    }
  }

  next_opcode = read_opcode (int_data->pos++);
  JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);
  JERRY_ASSERT (next_opcode.data.meta.type == OPCODE_META_TYPE_END_TRY_CATCH_FINALLY);

  return try_completion;
} /* opfunc_try */
