/** @file
 *
 *  Copyright (c), 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD License
 *  which accompanies this distribution.  The full text of the license may be found at
 *  http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
 **/

#include "HypDxe.h"
#include "ArmDefs.h"

#define NT_MIN_ILM       0xFFFFF80000000000ul
#define NT_MAX_ILM       0xFFFFF87FFFFFFFFFul
#define WS_BUILD_UNKNOWN 0

#define KPCR_LOCK_ARRAY(KPCR) *((UINT64 *) (U8P((KPCR)) + 0x28))
#define KPCR_VER_MAJOR(KPCR) *((UINT16 *) (U8P((KPCR)) + 0x3c))
#define KPCR_VER_MINOR(KPCR) *((UINT16 *) (U8P((KPCR)) + 0x3e))
#define KPCR_KD_VER_BLOCK(KPCR) *((UINT64 *) (U8P((KPCR)) + 0x80))

typedef struct {
  UINT16 MajorVersion;
  UINT16 MinorVersion;
  UINT8 ProtocolVersion;
  UINT8 KdSecondaryVersion;
  UINT16 Flags;

  UINT16 MachineType;
  UINT8 MaxPacketType;
  UINT8 MaxStateChange;
  UINT8 MaxManipulate;
  UINT8 Simulation;
  UINT16 Unused;

  UINT64 KernBase;
  UINT64 PsLoadedModuleList;
  UINT64 DebuggerDataList;
} DBGKD_GET_VERSION64;

STATIC UINTN mWSBuild;
STATIC BOOLEAN mWSCanPatch;


STATIC HVA
Probe(
  IN  GVA VA
  )
{
  UINT64 Par;
  UINT64 ParSaved;

  /*
   * Preserve EL1 PAR.
   */
  ReadSysReg(ParSaved, par_el1);
  ISB();

  ATS12E1R(VA);
  ISB();

  ReadSysReg(Par, par_el1);
  if (PAR_IS_BAD(Par)) {
    goto done;
  }

  return MPA_2_HVA((PAR_2_ADDR(Par) | X(VA, 0, 11)));
done:
  ISB();
  WriteSysReg(par_el1, ParSaved);
  return INVALID_HVA;
}


STATIC HVA
ProbeIgnZero(
  IN  GVA VA
  )
{
  if (VA == 0) {
    return INVALID_HVA;
  }

  return Probe(VA);
}


STATIC BOOLEAN
HypWSProbeKdVerBlock(
  IN  DBGKD_GET_VERSION64 *KdVer,
  IN  GVA KnownKernBase
  )
{
  if (KdVer == NULL) {
    return FALSE;
  }

  if (KdVer->MajorVersion != 0xf &&
      KdVer->MajorVersion != 0xc) {
    return FALSE;
  }

  if (KdVer->MachineType != 0xAA64) {
    return FALSE;
  }

  if (KnownKernBase != 0 &&
      KnownKernBase != KdVer->KernBase) {
    return FALSE;
  }

  DEBUG((EFI_D_INFO, "Detected arm64%a build %u, NT base = 0x%lx\n",
         KdVer->MajorVersion == 0xf ? "fre" : "chk",
         KdVer->MinorVersion, KdVer->KernBase));

  mWSBuild = KdVer->MinorVersion;
  return TRUE;
}


STATIC BOOLEAN
HypWSFindKdVerBlock(
  IN  HVA KPCR,
  IN  GVA KPCRVA
  )
{
  GVA ProbeVA = KPCRVA;
  const UINT32 Sig = UN('K') << 0 |
    UN('D') << 8 |
    UN('B') << 16 |
    UN('G') << 24;

  for (ProbeVA = KPCRVA;
       ProbeVA < NT_MAX_ILM;
       ProbeVA += EFI_PAGE_SIZE) {
    UINT32 *Page = HVA_2_P(Probe(ProbeVA));
    UINT32 *End = Page + EFI_PAGE_SIZE / sizeof(UINT32);

    if (Page == NULL || (HVA) Page < KPCR) {
      continue;
    }

    for (; Page < End; Page++) {
      HVA KdVerSearchBase;
      HVA KdVerSearchEnd;
      GVA KernBase;

      if (*Page != Sig) {
        continue;
      }

      if (!IS_ALIGNED((Page + 2), sizeof(UINT64))) {
        continue;
      }

      KernBase = *(UINT64 *) (Page + 2);
      if ((KernBase & NT_MIN_ILM) != NT_MIN_ILM) {
        /*
         * This is for sure not _KDDEBUGGER_DATA64.KernBase.
         */
        continue;
      }

      /*
       * This is nasty and hacky - yes, but it's miles faster than
       * being proper and AT()ing every GVA around ProbeVA. These
       * are kernel VAs and they are backed by linear GPAs/HVAs.
       * Hopefully we never have to manually scanning from
       * KernBase on up for something that could look like
       * DBGKD_GET_VERSION64. Ugh.
       */
      KdVerSearchBase = A_DOWN((HVA) Page, EFI_PAGE_SIZE) - 10 * EFI_PAGE_SIZE;
      KdVerSearchEnd = A_UP((HVA) Page, EFI_PAGE_SIZE) + 10 * EFI_PAGE_SIZE;

      while (KdVerSearchBase < KdVerSearchEnd) {
        if (HypWSProbeKdVerBlock(HVA_2_P(KdVerSearchBase),
                                 KernBase)) {
          DEBUG((EFI_D_VERBOSE, "Matched DBGKD_GET_VERSION64 @ lx relative to %p\n",
                 KdVerSearchBase, Page));
          return TRUE;
        }

        KdVerSearchBase++;
      }
    }
  }

  return FALSE;
}


STATIC BOOLEAN
HypWSSupported(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *Context
  )
{
  HVA KPCR;
  GVA KPCRVA;
  VOID *KdVersionBlock;

  if (!mWSCanPatch) {
    return FALSE;
  }

  /*
   * ntddk.h defines KIPCR() by masking
   * off low bits.
   */
  ReadSysReg(KPCRVA, tpidr_el1);
  KPCRVA &= ~0xFFFul;
  if (KPCRVA != Context->X18) {
    /*
     * Windows uses TPIDR_EL1 and x18 in kernel
     * mode to point to the KPCR.
     */
    return FALSE;
  }

  if (KPCRVA < NT_MIN_ILM ||
      KPCRVA > NT_MAX_ILM) {
    /*
     * Outside of the initial loader mapping ranges.
     */
    return FALSE;
  }

  KPCR = Probe(KPCRVA);
  if (KPCR == INVALID_HVA) {
    DEBUG((EFI_D_ERROR, "Couldn't probe KPCR\n"));
    return FALSE;
  }

  if (KPCR_VER_MAJOR(KPCR) != 1 ||
      KPCR_VER_MINOR(KPCR) != 1) {
    /*
     * The two fields that never change...
     */
    return FALSE;
  }

  KdVersionBlock = HVA_2_P(ProbeIgnZero(KPCR_KD_VER_BLOCK(KPCR)));
  if (HypWSProbeKdVerBlock(KdVersionBlock, 0)) {
    return TRUE;
  }

  DEBUG((EFI_D_INFO, "Searching for DBGKD_GET_VERSION64...\n"));
  if (!HypWSFindKdVerBlock(KPCR, KPCRVA)) {
    DEBUG((EFI_D_ERROR, "Cannot identify build, try with debug enabled in BCD\n"));
  }

  /*
   * While some patches may rely on knowing the build, others
   * may try matching a known instruction sequence, and still
   * work.
   */
  return TRUE;
}


STATIC UINT32 *
SeqSearch(
  IN  UINT32 *Page,
  IN  UINT32 *End,
  IN  UINT32 *Seq,
  IN  UINTN  SeqEles
  )
{
  UINTN Index;

  while (Page + SeqEles < End) {
    for (Index = 0; Index < SeqEles; Index++) {
      if (Page[Index] != Seq[Index]) {
        break;
      }
    }

    if (Index == SeqEles) {
      return Page;
    }

    Page += Index + 1;
  }

  return NULL;
}


STATIC GVA
FindHalpInterruptRegisterControllerLoc(
  IN  GVA From
  )
{
  /*
   * See comment in PatchHalpInterruptRegisterController.
   */
  static UINT32 Seq[] = {
    0x51000da8, 0x7100051f, 0x54000069
  };

  GVA ProbeVA = A_DOWN(From, EFI_PAGE_SIZE);
  /*
   * Hypothetically, the matching could be fuzzier,
   * accepting any registers and branch offsets, under
   * the theory that small additions or removals to the
   * function may have changed the register assignments
   * or branch distances, but won't change the sequence
   * of opcodes themselves or the immediates used.
   *
   * Today I'll just memcmp.
   */
  for (ProbeVA = From;
       ProbeVA < NT_MAX_ILM;
       ProbeVA += EFI_PAGE_SIZE) {
    UINT32 *Page = HVA_2_P(Probe(ProbeVA));
    UINT32 *End = Page + EFI_PAGE_SIZE / sizeof(UINT32);
    UINT32 *Cur;

    if (Page == NULL) {
      /*
       * We're looking inside the HAL, unmapped pages means
       * we've failed.
       */
      break;
    }

    Cur = SeqSearch(Page, End, Seq, ELES(Seq));
    if (Cur != NULL) {
      return UN(Cur + ELES(Seq) - 1) - UN(Page) + ProbeVA;
    }
  }
  return INVALID_GVA;
}


STATIC
BOOLEAN
PatchHalpInterruptRegisterController(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *Context
  )
{
  UINT32 *Insn;
  GVA PC = Context->ELR;

  /*
   * hal!HalpPowerInvokeSmc:
   * 0x0: d10043ff sub  sp, sp #0x10
   * 0x4: f506b004 str  x7, [sp]
   * 0x8: d4000003 smc  #0            <-- PC
   * 0xa: f94003e7 ldr  x7, [sp]
   * ...
   */

  if (mWSBuild >= 17125 &&
      mWSBuild <= 17134) {
    PC += 0x7684;
  } else if (mWSBuild == 17672) {
    PC += 0x7d4c;
  } else {
    /*
     * Unknown build Let's try finding
     * the area of interest..
     */
    DEBUG((EFI_D_ERROR, "Unknown build %u, trying to match\n",
           mWSBuild));
    PC = FindHalpInterruptRegisterControllerLoc(PC);
  }

  if (PC == INVALID_GVA) {
    DEBUG((EFI_D_ERROR, "Couldn't locate hal!HalpInterruptRegisterController patch site\n"));
    return FALSE;
  }

  /*
   * hal!HalpInterruptRegisterController:
   * ...
   * 0x64: 51000da8 sub  w8, w13, #3
   * 0x68: 7100051f cmp  w8, #1
   * 0x6c: 54000069 bls  <func>+0x78  <-- PC
   *
   * Patch the `bls` (54000069) to `b` (14000003).
   */

  Insn = HVA_2_P(Probe(PC));
  if (*Insn == 0x14000003) {
    DEBUG((EFI_D_ERROR, "Already patched?\n"));
    return TRUE;
  } else if (*Insn != 0x54000069) {
    DEBUG((EFI_D_ERROR, "Invalid hal!HalpInterruptRegisterController patch site (0x%x)\n", *Insn));
    return FALSE;
  }

  *Insn = 0x14000003;
  return TRUE;
}


VOID
HypWSTryPatch(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *Context
  )
{
  BOOLEAN PatchStatus = FALSE;

  if (!HypWSSupported(Context)) {
    /*
     * Not Windows.
     */
    goto done;
  }

  PatchStatus = PatchHalpInterruptRegisterController(Context);
  if (!PatchStatus) {
    DEBUG((EFI_D_ERROR, "Patching WoA failed, good luck!\n"));
  }

done:
  mWSCanPatch = FALSE;
}


VOID
HypWSInit(
  VOID
  )
{
  mWSBuild = WS_BUILD_UNKNOWN;
  mWSCanPatch = TRUE;
}
