//===-- OptionGroupUUID.h ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_OptionGroupUUID_h_
#define liblldb_OptionGroupUUID_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Interpreter/OptionValueUUID.h"
#include "lldb/Interpreter/Options.h"

namespace lldb_private {

//-------------------------------------------------------------------------
// OptionGroupUUID
//-------------------------------------------------------------------------

class OptionGroupUUID : public OptionGroup {
public:
  OptionGroupUUID();

  ~OptionGroupUUID() override;

  uint32_t GetNumDefinitions() override;

  const OptionDefinition *GetDefinitions() override;

  Error SetOptionValue(uint32_t option_idx, const char *option_value,
                       ExecutionContext *execution_context) override;

  void OptionParsingStarting(ExecutionContext *execution_context) override;

  const OptionValueUUID &GetOptionValue() const { return m_uuid; }

protected:
  OptionValueUUID m_uuid;
};

} // namespace lldb_private

#endif // liblldb_OptionGroupUUID_h_
