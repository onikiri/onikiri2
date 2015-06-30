// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#ifndef __PPC64LINUX_PPC64CONVERTER_H__
#define __PPC64LINUX_PPC64CONVERTER_H__

#include "Emu/PPC64Linux/PPC64Info.h"
#include "Emu/PPC64Linux/PPC64Decoder.h"
#include "Emu/PPC64Linux/PPC64OpInfo.h"
#include "Emu/Utility/CommonConverter.h"

namespace Onikiri {

    namespace PPC64Linux {

        // PowerPCの命令を，OpInfo の列に変換する
        struct PPC64ConverterTraits {
            typedef PPC64OpInfo OpInfoType;
            typedef PPC64Decoder DecoderType;

            typedef u32 CodeWordType;
            static const int MaxOpInfoDefs = 3;
            static const int MaxDstOperands = PPC64OpInfo::MaxDstRegCount;
            static const int MaxSrcOperands = 6;    // SrcReg と SrcImm の合計
        };

        // Alphaの命令を，OpInfo の列に変換する
        class PPC64Converter : public EmulatorUtility::CommonConverter<PPC64ConverterTraits>
        {
        public:
            PPC64Converter();
            virtual ~PPC64Converter();

        private:
            // CommonConverter のカスタマイズ
            virtual bool IsZeroReg(int reg) const;
            virtual std::pair<OperandType, int> GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const;
            virtual int GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const;
            virtual const OpDef* GetOpDefUnknown() const;

            // OpDef
            static OpDef m_OpDefsBase[];
            static OpDef m_OpDefsSplitLoadStore[];
            static OpDef m_OpDefsNonSplitLoadStore[];
            static OpDef m_OpDefUnknown;

            static void PPC64UnknownOperation(EmulatorUtility::OpEmulationState* opState);
        };
    } // namespace PPC64Linux
} // namespace Onikiri

#endif
