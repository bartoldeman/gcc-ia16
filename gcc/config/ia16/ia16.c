/* Subroutines used during code generation for Intel 16-bit x86.
   Copyright (C) 2005-2017 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>
   Changes by Andrew Jenner <andrew@codesourcery.com>
   Very preliminary IA-16 far pointer support and other changes by TK Chia

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation; either version 3 of the License, or (at your option) any
   later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* I have no or little idea how many and which of these headers files I need
 * to include.  So don't look here if you don't have a clue either.
 *
 * FIXME: Docs say config.h includes tm.h, but it doesn't.
 */
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "cfgloop.h"
#include "df.h"
#include "tm_p.h"
#include "expmed.h"
#include "optabs.h"
#include "regs.h"
#include "emit-rtl.h"
#include "diagnostic.h"
#include "fold-const.h"
#include "calls.h"
#include "stor-layout.h"
#include "varasm.h"
#include "output.h"
#include "explow.h"
#include "expr.h"
#include "gimple-iterator.h"
#include "tree-vectorizer.h"
#include "builtins.h"
#include "cfgrtl.h"
#include "libiberty.h"
#include "hashtab.h"
#include "print-tree.h"
#include "langhooks.h"

/* This file should be included last.  */
#include "target-def.h"

/* The Global targetm Variable.  */
/* Part 1.  Part 2 is at the end of this file.  */

/* Storage Layout.  */

/* Layout of Source Language Data Types */
#undef  TARGET_DEFAULT_SHORT_ENUMS
#define TARGET_DEFAULT_SHORT_ENUMS	hook_bool_void_true

/* How Values Fit in Registers */
/* This is complex enough to warrant a table.
 * Only 8-bit registers can hold QImode values. TODO: Lift this restriction.
 * Multibyte values are forced to start in even registers because the hw
 * makes this much easier and faster and some insn patterns may rely on it.
 * Don't put 8-byte values in bx because it would take all BASE_REGS.
 * Don't put 8-byte values in bx or dx because subreg_get_info can't handle it.
 * find_valid_class() fails if we don't take the ending register into account.
 * subreg_get_info() dies on libssp/gets-chk.c if !H_R_M_O (SP_REG, HImode).
 * Disallow register size changes unless HARD_REGNO_NREGS_HAS_PADDING.
 * CCmode is 4 bytes.
 */
unsigned char ia16_hard_regno_nregs[17][FIRST_PSEUDO_REGISTER] =
{
/* size     cl  ch  al  ah  dl  dh  bl  bh  si  di  bp  es  ds  sp  cc  ss  cs  ap */
/*  0 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  1 */  {  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  2 */  {  2,  0,  2,  0,  2,  0,  2,  0,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1 },
/*  3 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  4 */  {  4,  0,  4,  0,  4,  0,  3,  0,  2,  2,  2,  2,  0,  0,  1,  0,  0,  0 },
/*  5 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  6 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  7 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  8 */  {  8,  0,  7,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  9 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 10 */  {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 11 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 12 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 13 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 14 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 15 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 16 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
};

/* Register Classes.  */
enum reg_class const ia16_regno_class[FIRST_PSEUDO_REGISTER] = {
	/*  0 cl */ CL_REGS,
	/*  1 ch */ CX_REGS,
	/*  2 al */ AL_REGS,
	/*  3 ah */ AH_REGS,
	/*  4 dl */ DL_REGS,
	/*  5 dh */ DH_REGS,
	/*  6 bl */ BX_REGS,
	/*  7 bh */ BX_REGS,
	/*  8 si */ SI_REGS,
	/*  9 di */ DI_REGS,
	/* 10 bp */ BP_REGS,
	/* 11 es */ ES_REGS,
	/* 12 ds */ DS_REGS,
	/* 13 sp */ HI_REGS,
	/* 14 cc */ ALL_REGS,
	/* 15 ss */ SEGMENT_REGS,
	/* 16 cs */ SEGMENT_REGS,
	/* 17 ap */ ALL_REGS,
};

/* Processor target table, indexed by processor number */
struct ptt
{
  const struct processor_costs *cost;           /* Processor costs */
  int features;
};

#undef	TARGET_PREFERRED_RELOAD_CLASS
#define	TARGET_PREFERRED_RELOAD_CLASS ia16_preferred_reload_class

static reg_class_t
ia16_preferred_reload_class (rtx x ATTRIBUTE_UNUSED, reg_class_t rclass)
{
  /* Try to avoid roping in %ds.  */
  switch (rclass)
    {
    case SEGMENT_REGS:
      return ES_REGS;

    case SEG_GENERAL_REGS:
      return TARGET_PROTECTED_MODE ? GENERAL_REGS : ES_GENERAL_REGS;

    default:
      return rclass;
    }
}

#undef  TARGET_SECONDARY_RELOAD
#define TARGET_SECONDARY_RELOAD ia16_secondary_reload

/* Handle loading a constant into a segment register.  Handle loading a frame
 * pointer offset into a segment register.  This is also where copies between
 * segment registers should be handled, if needed.  All of this needs an
 * intermediate register of class GENERAL_REGS.  */
static reg_class_t
ia16_secondary_reload (bool in_p, rtx x, reg_class_t reload_class,
		       enum machine_mode reload_mode ATTRIBUTE_UNUSED,
		       secondary_reload_info *sri ATTRIBUTE_UNUSED)
{
  if (in_p
      && (reload_class == SEG_GENERAL_REGS
	  || reload_class == SEGMENT_REGS
	  || reload_class == ES_REGS
	  || reload_class == DS_REGS)
      && (CONSTANT_P (x) || PLUS == GET_CODE (x)))
    {
      return (GENERAL_REGS);
    }
  return (NO_REGS);
}

#undef TARGET_LRA_P
#define TARGET_LRA_P hook_bool_void_true

#undef TARGET_CANNOT_SUBSTITUTE_MEM_EQUIV_P
#define TARGET_CANNOT_SUBSTITUTE_MEM_EQUIV_P ia16_cannot_substitute_mem_equiv_p

/* Return true if SUBST can't safely replace its equivalent during RA.  */
static bool
ia16_cannot_substitute_mem_equiv_p (rtx subst)
{
  if (!MEM_P  (subst))
    return false;
  rtx e = XEXP (subst, 0);
  if (GET_CODE (e) != PLUS)
    return false;
  rtx x = XEXP (e, 0);
  if (GET_CODE (x) == PLUS)
    return true;
  rtx y = XEXP (e, 1);
  return GET_CODE (x) == REG && GET_CODE (y) == REG;
}

/* Convenience function --- say whether register r belongs to class c.  */
int
ia16_regno_in_class_p (unsigned r, unsigned c)
{
  return r < FIRST_PSEUDO_REGISTER
	 && TEST_HARD_REG_BIT (reg_class_contents[c], r);
}

/* Returns non-zero if register r must be saved by a function, zero if not.  */
/* Always returns zero for upper halves of 16-bit registers
 * (i.e. ah, dh, bh or ch).  */
static int
ia16_save_reg_p (unsigned int r)
{
  if (r == BP_REG)
    return frame_pointer_needed;
  if (! ia16_regno_in_class_p (r, QI_REGS))
    return (df_regs_ever_live_p (r) && !call_used_regs[r]);
  if (ia16_regno_in_class_p (r, UP_QI_REGS))
    return (0);
  return ((df_regs_ever_live_p (r + 0) && !call_used_regs[r + 0]) ||
	  (df_regs_ever_live_p (r + 1) && !call_used_regs[r + 1]));
}

/* Given the address ADDR of a function, return its declaration node, or
   NULL_TREE if none can be found.
   FIXME: this may not be 100% reliable...  */
static tree
ia16_get_function_decl_for_addr (rtx addr)
{
  switch (GET_CODE (addr))
    {
    case MEM:
      return MEM_EXPR (addr);
      break;

    case REG:
      return REG_EXPR (addr);
      break;

    case SYMBOL_REF:
      return SYMBOL_REF_DECL (addr);
      break;

    default:
      return NULL_TREE;
    }
}

/* Given the address ADDR of a function, return its type node, or NULL_TREE
   if no type node can be found.  */
static tree
ia16_get_function_type_for_addr (rtx addr)
{
  tree decl = ia16_get_function_decl_for_addr (addr), type;

  if (! decl)
    return NULL_TREE;

  type = TREE_TYPE (decl);
  while (POINTER_TYPE_P (type))
    type = TREE_TYPE (type);

  return type;
}

/* Return true iff TYPE is a type for a far function (which returns with
   `lret').  Currently a function is considered to be "far" if the function
   type itself is in __far space.  The `-mfar-function-if-far-return-type'
   switch will also cause a __far on the return type to magically become a
   __far on the function itself.  */
static int
ia16_far_function_type_p (const_tree funtype)
{
  return TYPE_ADDR_SPACE (funtype) == ADDR_SPACE_FAR;
}

/* Return true iff we are currently compiling a far function.  */
int
ia16_in_far_function_p (void)
{
  return cfun && cfun->decl
	 && ia16_far_function_type_p (TREE_TYPE (cfun->decl));
}

/* Return true iff TYPE is a type for a function which assumes that %ds
   points to the program's data segment on function entry.

   If %ds is a call-saved register (-fcall-saved-ds), this is always false.  */
int
ia16_ds_data_function_type_p (const_tree funtype)
{
  tree attrs;

  if (! call_used_regs[DS_REG])
    return 0;

  attrs = TYPE_ATTRIBUTES (funtype);

  if (TARGET_ASSUME_DS_DATA)
    return ! attrs || ! lookup_attribute ("no_assume_ds_data", attrs);
  else
    return attrs && lookup_attribute ("assume_ds_data", attrs);
}

/* Return true iff TYPE is a type for a function which follows the default
   ABI for %ds --- which says that %ds
     * is considered (by the GCC middle-end) to be a call-used register,
     * is assumed to point to the program's data segment on function entry,
     * and is restored to the data segment on function exit.  */
static int
ia16_default_ds_abi_function_type_p (const_tree funtype)
{
  return TARGET_ALLOCABLE_DS_REG
	 && ia16_ds_data_function_type_p (funtype);
}

static int
ia16_in_ds_data_function_p (void)
{
  if (cfun && cfun->decl)
    return ia16_ds_data_function_type_p (TREE_TYPE (cfun->decl));
  else
    return TARGET_ASSUME_DS_DATA;
}

#define TARGET_DEFAULT_DS_ABI	(TARGET_ALLOCABLE_DS_REG \
				 && call_used_regs[DS_REG] \
				 && TARGET_ASSUME_DS_DATA)

static int
ia16_in_default_ds_abi_function_p (void)
{
  if (cfun && cfun->decl)
    return ia16_default_ds_abi_function_type_p (TREE_TYPE (cfun->decl));
  else
    return TARGET_DEFAULT_DS_ABI;
}

static int
ia16_ds_data_function_rtx_p (rtx addr)
{
  tree type = ia16_get_function_type_for_addr (addr);
  if (type)
    return ia16_ds_data_function_type_p (type);
  else
    return TARGET_ASSUME_DS_DATA;
}

static int
ia16_default_ds_abi_function_rtx_p (rtx addr)
{
  tree type = ia16_get_function_type_for_addr (addr);
  if (type)
    return ia16_default_ds_abi_function_type_p (type);
  else
    return TARGET_DEFAULT_DS_ABI;
}

static int
ia16_near_section_function_type_p (const_tree funtype)
{
  tree attrs = TYPE_ATTRIBUTES (funtype);
  return attrs && lookup_attribute ("near_section", attrs);
}

static int
ia16_far_section_function_type_p (const_tree funtype)
{
  tree attrs = TYPE_ATTRIBUTES (funtype);
  return attrs && lookup_attribute ("far_section", attrs);
}

static int
ia16_in_far_section_function_p (void)
{
  if (cfun && cfun->decl)
    return ia16_far_section_function_type_p (TREE_TYPE (cfun->decl));
  else
    return 0;
}

/* Basic Stack Layout */
/* Right after the function prologue, before elimination, we have:
   argument N
   argument N - 1
   ...
   argument 1
   return address		<-- argument pointer
   saved reg 1
   saved reg 2
   ...
   saved reg N
   saved reg bp?		<-- frame pointer
   local variables start
   local variables end		<-- stack pointer
   function outgoing arguments
*/

/* Calculates the offset from the argument pointer to the first argument.
 * When a frame pointer is needed and bp must be saved, it is saved before the
 * frame is created.  This increases the offset to the parameters by 2 bytes.
 * A __far call would add another 2 bytes.
 * FIXME: Docs: This is called before register allocation.
 */
HOST_WIDE_INT
ia16_first_parm_offset (tree fundecl)
{
  /* Start off with 2 bytes to skip over the saved pc register.  */
  HOST_WIDE_INT offset = GET_MODE_SIZE (Pmode);

  /* Add 2 bytes if this is a far function.  */
  if (ia16_far_function_type_p (TREE_TYPE (fundecl)))
    offset += GET_MODE_SIZE (Pmode);

  return (offset);
}

/* Eliminating Frame Pointer and Arg Pointer */
/* Calculates the difference between the argument pointer and the frame
 * pointer immediately after the function prologue.  This should be kept int
 * sync with the prologue pattern.
 */
static HOST_WIDE_INT
ia16_initial_arg_pointer_offset (void)
{
  HOST_WIDE_INT offset = 0;
  unsigned int i;

  /* Add two bytes for each register saved.  */
  for (i = 0; i <= LAST_ALLOCABLE_REG; i ++)
    {
      if (ia16_save_reg_p (i))
	offset += GET_MODE_SIZE (HImode);
    }
  return offset;
}

/* Calculates the difference between the frame pointer and the stack pointer
 * values immediately after the function prologue.  This should be kept in sync
 * with the prologue pattern.
 */
HOST_WIDE_INT
ia16_initial_frame_pointer_offset (void)
{
  HOST_WIDE_INT offset;

  offset = get_frame_size ();

  return offset;
}

HOST_WIDE_INT
ia16_initial_elimination_offset (unsigned int from, unsigned int to)
{
  if (ARG_POINTER_REGNUM == from && FRAME_POINTER_REGNUM == to)
    return (ia16_initial_arg_pointer_offset ());

  if (FRAME_POINTER_REGNUM == from && STACK_POINTER_REGNUM == to)
    return (ia16_initial_frame_pointer_offset ());

  if (ARG_POINTER_REGNUM == from && STACK_POINTER_REGNUM == to)
    return (ia16_initial_arg_pointer_offset ()
	  + ia16_initial_frame_pointer_offset ());

  gcc_unreachable ();
}

/* Passing Arguments in Registers */
#undef  TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG ia16_function_arg

static rtx
ia16_function_arg (cumulative_args_t cum_v, machine_mode mode,
		   const_tree type ATTRIBUTE_UNUSED,
		   bool named)
{
  CUMULATIVE_ARGS *cum;

  if (! named)
    return NULL;

  cum = get_cumulative_args (cum_v);

  switch (mode)
    {
    case QImode:
    case HImode:
    case V2QImode:
      switch (*cum)
	{
	case 0:
	  return gen_rtx_REG (mode, A_REG);
	case 1:
	  return gen_rtx_REG (mode, D_REG);
	case 2:
	  return gen_rtx_REG (mode, C_REG);
	default:
	  return NULL;
	}

    case SImode:
    case SFmode:
      switch (*cum)
      {
      case 0:				/* %dx:%ax */
	return gen_rtx_REG (mode, A_REG);
      case 1:				/* %cx:%dx */
	{
	  rtx dx = gen_rtx_REG (HImode, D_REG);
	  rtx list0 = gen_rtx_EXPR_LIST (VOIDmode, dx, const0_rtx);
	  rtx cx = gen_rtx_REG (HImode, C_REG);
	  rtx list1 = gen_rtx_EXPR_LIST (VOIDmode, cx, const2_rtx);
	  return gen_rtx_PARALLEL (mode, gen_rtvec (2, list0, list1));
	}
      default:
	return NULL;
      }

    default:
      return NULL;
    }
}

static enum call_parm_cvt_type
ia16_get_call_parm_cvt (const_tree type)
{
  if (type)
    {
      tree attrs = TYPE_ATTRIBUTES (type);

      if (attrs != NULL_TREE)
	{
	  if (lookup_attribute ("cdecl", attrs))
	    return CALL_PARM_CVT_CDECL;
	  else if (lookup_attribute ("stdcall", attrs))
	    return CALL_PARM_CVT_STDCALL;
	  else if (lookup_attribute ("regparmcall", attrs))
	    return CALL_PARM_CVT_REGPARMCALL;
	}
    }

  return (enum call_parm_cvt_type) target_call_parm_cvt;
}

static bool
ia16_regparmcall_function_type_p (const_tree fntype)
{
  return ia16_get_call_parm_cvt (fntype) == CALL_PARM_CVT_REGPARMCALL;
}

void
ia16_init_cumulative_args (CUMULATIVE_ARGS *cum, const_tree fntype,
			   rtx libname ATTRIBUTE_UNUSED,
			   const_tree fndecl ATTRIBUTE_UNUSED,
			   int n_named_args ATTRIBUTE_UNUSED)
{
  if (! ia16_regparmcall_function_type_p (fntype) || stdarg_p (fntype))
    *cum = 3;
  else
    *cum = 0;
}

#undef  TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE ia16_function_arg_advance

static void
ia16_function_arg_advance (cumulative_args_t cum_v, machine_mode mode,
                           const_tree type ATTRIBUTE_UNUSED,
			   bool named ATTRIBUTE_UNUSED)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);

  if (*cum >= 3 || ! named)
    {
      *cum = 4;
      return;
    }

  switch (mode)
    {
    case QImode:
    case HImode:
    case V2QImode:
      ++*cum;
      return;

    case SImode:
    case SFmode:
      switch (*cum)
	{
	case 0:
	  *cum = 2;
	  return;
	case 1:
	  *cum = 3;
	  return;
	default:
	  *cum = 4;
	  return;
	}

    default:
      *cum = 4;
      return;
    }
}

#undef  TARGET_VECTOR_MODE_SUPPORTED_P
#define TARGET_VECTOR_MODE_SUPPORTED_P ia16_vector_mode_supported_p
static bool
ia16_vector_mode_supported_p (enum machine_mode mode)
{
  return (mode == V2QImode);
}

/* How Scalar Function Values Are Returned */
#undef  TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE ia16_function_value

static rtx
ia16_function_value (const_tree ret_type,
		     const_tree fn_decl_or_type ATTRIBUTE_UNUSED,
		     bool outgoing ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (TYPE_MODE (ret_type), A_REG);
}

#undef	TARGET_LIBCALL_VALUE
#define	TARGET_LIBCALL_VALUE	ia16_libcall_value

static rtx
ia16_libcall_value (machine_mode mode, const_rtx fun ATTRIBUTE_UNUSED)
{
  if (! HARD_REGNO_MODE_OK (A_REG, mode))
    return NULL;

  return gen_rtx_REG (mode, A_REG);
}

#undef	TARGET_FUNCTION_VALUE_REGNO_P
#define	TARGET_FUNCTION_VALUE_REGNO_P ia16_function_value_regno_p

static bool
ia16_function_value_regno_p (unsigned regno)
{
  return regno == A_REG;
}

/* How Large Values Are Returned */
/* FIXME Documentation: This is for scalar values as well.  */
#undef  TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY ia16_return_in_memory

static bool
ia16_return_in_memory (const_tree type, const_tree fntype ATTRIBUTE_UNUSED)
{
  /* Return in memory if it's larger than 4 bytes or BLKmode.
   * TODO: Increase this to 8 bytes or so.  Doing so requires more call
   * used registers or requires the prologue and epilogue patterns to not
   * touch the return value registers.  */
   return (TYPE_MODE (type) == BLKmode || int_size_in_bytes (type) > 4);
}

#undef	TARGET_GET_RAW_RESULT_MODE
#define	TARGET_GET_RAW_RESULT_MODE ia16_get_raw_result_mode

static machine_mode
ia16_get_raw_result_mode (int regno)
{
  switch (regno)
    {
    case A_REG:
      return SImode;
    default:
      gcc_unreachable ();
    }
}

#undef	TARGET_GET_RAW_ARG_MODE
#define	TARGET_GET_RAW_ARG_MODE ia16_get_raw_arg_mode

static machine_mode
ia16_get_raw_arg_mode (int regno)
{
  switch (regno)
    {
    case A_REG:
    case D_REG:
    case C_REG:
      return HImode;
    case DS_REG:
      return VOIDmode;
    default:
      gcc_unreachable ();
    }
}

/* Permitting tail calls */
#undef  TARGET_FUNCTION_OK_FOR_SIBCALL
#define TARGET_FUNCTION_OK_FOR_SIBCALL ia16_function_ok_for_sibcall
static bool
ia16_function_ok_for_sibcall (tree decl, tree exp ATTRIBUTE_UNUSED)
{
  /* For now, only allow sibcalling known functions.
     TODO: Try relaxing this.  */
  if (! decl)
    return false;

  /* If the current function is a far function, but the callee is not a far
     function, then do not allow a sibcall.  */
  if (ia16_in_far_function_p ()
      && ! ia16_far_function_type_p (TREE_TYPE (decl)))
    return false;

  /* Ditto if the current function is a near function but not the callee.  */
  if (! ia16_in_far_function_p ()
      && ia16_far_function_type_p (TREE_TYPE (decl)))
    return false;

  return true;
}

/* Passing Function Arguments on the Stack */
#undef	TARGET_RETURN_POPS_ARGS
#define	TARGET_RETURN_POPS_ARGS ia16_return_pops_args

/* Calling convention flags as returned by ia16_get_callcvt (.).

   These should only cover specifiers, qualifiers, and attributes that
   affect what happens when one function tries to call another function. 
   Attributes which only affect how code behaves _within_ a function should
   not be included.  */
#define IA16_CALLCVT_CDECL		0x01
#define IA16_CALLCVT_STDCALL		0x02
#define IA16_CALLCVT_REGPARMCALL	0x04
#define IA16_CALLCVT_FAR		0x08
#define IA16_CALLCVT_DS_DATA		0x10
#define IA16_CALLCVT_NEAR_SECTION	0x20

static unsigned
ia16_get_callcvt (const_tree type)
{
  tree attrs;
  unsigned callcvt;

  switch (ia16_get_call_parm_cvt (type))
    {
    case CALL_PARM_CVT_CDECL:
      callcvt = IA16_CALLCVT_CDECL;
      break;
    case CALL_PARM_CVT_STDCALL:
      callcvt = IA16_CALLCVT_STDCALL;
      break;
    case CALL_PARM_CVT_REGPARMCALL:
      callcvt = IA16_CALLCVT_REGPARMCALL;
      break;
    default:
      gcc_unreachable ();
    }

  if (! type)
    return callcvt;

  attrs = TYPE_ATTRIBUTES (type);

  if (attrs != NULL_TREE)
    {
      if (lookup_attribute ("near_section", attrs))
	callcvt |= IA16_CALLCVT_NEAR_SECTION;
    }

  if (ia16_ds_data_function_type_p (type))
    callcvt |= IA16_CALLCVT_DS_DATA;

  if (ia16_far_function_type_p (type))
    callcvt |= IA16_CALLCVT_FAR;

  return callcvt;
}

static int
ia16_return_pops_args (tree fundecl ATTRIBUTE_UNUSED, tree funtype, int size)
{
  /* Note that the `-mrtd' or `-mregparmcall' calling convention will also be
     applied to libgcc library functions (e.g. __udivdi3).  This usually
     means that, if we compile code using `-mrtd'/`-mregparmcall', we will
     need a libgcc multilib compiled with the same option to link against our
     code.  */
  if (stdarg_p (funtype))
    return 0;

  switch (ia16_get_callcvt (funtype)
	  & (IA16_CALLCVT_STDCALL | IA16_CALLCVT_CDECL
	     | IA16_CALLCVT_REGPARMCALL))
    {
    case IA16_CALLCVT_STDCALL:
    case IA16_CALLCVT_REGPARMCALL:
      return size;
    case IA16_CALLCVT_CDECL:
      return 0;
    default:
      gcc_unreachable ();
    }
}

/* Defining target-specific uses of __attribute__ */
#undef	TARGET_ATTRIBUTE_TABLE
#define	TARGET_ATTRIBUTE_TABLE ia16_attribute_table

static tree
ia16_handle_cconv_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			     int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  switch (TREE_CODE (*node))
    {
    case FUNCTION_TYPE:
    case METHOD_TYPE:
    case FIELD_DECL:
    case TYPE_DECL:
      break;

    default:
      warning (OPT_Wattributes, "%qE attribute only applies to functions",
				name);
      *no_add_attrs = true;
      return NULL_TREE;
    }

  if (is_attribute_p ("stdcall", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && (lookup_attribute ("cdecl", attrs)
		    || lookup_attribute ("regparmcall", attrs)))
	{
	  error ("stdcall, cdecl, and regparmcall attributes are "
		 "not compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }
  else if (is_attribute_p ("cdecl", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && (lookup_attribute ("stdcall", attrs)
		    || lookup_attribute ("regparmcall", attrs)))
	{
	  error ("stdcall, cdecl, and regparmcall attributes are "
		 "not compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }
  else if (is_attribute_p ("regparmcall", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && (lookup_attribute ("cdecl", attrs)
		    || lookup_attribute ("stdcall", attrs)))
	{
	  error ("stdcall, cdecl, and regparmcall attributes are "
		 "not compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }

  if (! ia16_far_function_type_p (DECL_P (*node) ? TREE_TYPE (*node) : *node)
      && (is_attribute_p ("near_section", name)
	  || is_attribute_p ("far_section", name)))
    {
      warning (OPT_Wattributes, "%qE attribute directive ignored for "
				"non-far function", name);
      *no_add_attrs = true;
      return NULL_TREE;
    }
  /* The following attributes are ignored for non-far functions.  */
  else if (is_attribute_p ("near_section", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && lookup_attribute ("far_section", attrs))
	{
	  error ("near_section and far_section attributes are not compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }
  else if (is_attribute_p ("far_section", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && lookup_attribute ("near_section", attrs))
	{
	  error ("near_section and far_section attributes are not compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }

  if (! call_used_regs[DS_REG]
      && (is_attribute_p ("assume_ds_data", name)
	  || is_attribute_p ("no_assume_ds_data", name)))
    {
      warning (OPT_Wattributes, "%qE attribute directive ignored as %%ds is "
				"a call-saved register", name);
      *no_add_attrs = true;
      return NULL_TREE;
    }
  /* The following attributes are ignored if %ds is a call-saved register.  */
  else if (is_attribute_p ("assume_ds_data", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (attrs && lookup_attribute ("no_assume_ds_data", attrs))
	{
	  error ("assume_ds_data and no_assume_ds_data attributes are not "
		 "compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }
  else if (is_attribute_p ("no_assume_ds_data", name))
    {
      tree attrs = TYPE_ATTRIBUTES (*node);
      if (lookup_attribute ("assume_ds_data", attrs))
	{
	  error ("assume_ds_data and no_assume_ds_data attributes are not "
		 "compatible");
	  *no_add_attrs = true;
	}
      return NULL_TREE;
    }

  return NULL_TREE;
}

static const struct attribute_spec ia16_attribute_table[] =
{
  { "stdcall", 0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "cdecl",   0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "regparmcall",
	       0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "assume_ds_data",
	       0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "no_assume_ds_data",
	       0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "near_section",
	       0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { "far_section",
	       0, 0, false, true, true, ia16_handle_cconv_attribute, true },
  { NULL,      0, 0, false, false, false, NULL,			     false }
};

#undef	TARGET_COMP_TYPE_ATTRIBUTES
#define TARGET_COMP_TYPE_ATTRIBUTES ia16_comp_type_attributes

static int
ia16_comp_type_attributes (const_tree type1, const_tree type2)
{
  if (TREE_CODE (type1) != FUNCTION_TYPE
      && TREE_CODE (type1) != METHOD_TYPE)
    return 1;

  return ia16_get_callcvt (type1) == ia16_get_callcvt (type2);
}

/* Addressing Modes */

#undef  TARGET_ADDR_SPACE_ADDRESS_MODE
#define TARGET_ADDR_SPACE_ADDRESS_MODE ia16_as_address_mode

static enum machine_mode
ia16_as_address_mode (addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return HImode;
    /* A far address is actually a HImode value which is "coloured" with a
       segment term -- (plus:HI ...  (unspec:HI ...  UNSPEC_SEG_OVERRIDE)).
       See ia16_as_convert_weird_memory_address (...) below.  */
    case ADDR_SPACE_FAR:
      return HImode;
    default:
      gcc_unreachable ();
    }
}

#undef  TARGET_ADDR_SPACE_POINTER_MODE
#define TARGET_ADDR_SPACE_POINTER_MODE ia16_as_pointer_mode

static machine_mode
ia16_as_pointer_mode (addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return HImode;
    case ADDR_SPACE_FAR:
      return SImode;
    default:
      gcc_unreachable ();
    }
}

#undef  TARGET_ADDR_SPACE_VALID_POINTER_MODE
#define TARGET_ADDR_SPACE_VALID_POINTER_MODE ia16_as_valid_pointer_mode

static bool
ia16_as_valid_pointer_mode (machine_mode m, addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return m == HImode;
    case ADDR_SPACE_FAR:
      return m == SImode;
    default:
      gcc_unreachable ();
    }
}

bool
ia16_have_seg_override_p (rtx x)
{
  switch (GET_CODE (x))
    {
    case UNSPEC:
      /* Basis case.  */
      return (XINT (x, 1) == UNSPEC_SEG_OVERRIDE);
    case PLUS:
      return ia16_have_seg_override_p (XEXP (x, 0)) ||
	     ia16_have_seg_override_p (XEXP (x, 1));
    case MINUS:
      return ia16_have_seg_override_p (XEXP (x, 0));
    default:
      return false;
    }
}

#define REGNO_OK_FOR_SEGMENT_P(num) \
	ia16_regno_in_class_p ((num), SEGMENT_REGS)

#undef  TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P
#define TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P ia16_as_legitimate_address_p

static bool
ia16_as_legitimate_address_p (machine_mode mode ATTRIBUTE_UNUSED, rtx x,
			      bool strict, addr_space_t as)
{
  rtx r1, r2, r9;
  if (as != ADDR_SPACE_GENERIC && !ia16_have_seg_override_p (x))
    return false;
  if (CONSTANT_P (x))
    return true;
  if (!ia16_parse_address (x, &r1, &r2, NULL, &r9))
    return false;
  if (r9 != NULL_RTX && strict)
    {
      int r9no = REGNO (r9);
      if (r9no > LAST_HARD_REG && reg_renumber != 0)
	{
	  r9no = reg_renumber[r9no];
	  if (r9no < 0 || r9no > LAST_HARD_REG)
	    return false;
	}
      if (!REGNO_OK_FOR_SEGMENT_P (r9no))
        return false;
    }

  if (r1 == NULL_RTX)
    return true;
  int r1no = REGNO (r1);
  bool r1ok = true;
  if (strict)
    {
      r1ok = false;
      if (r1no > LAST_HARD_REG && reg_renumber != 0)
        {
          r1no = reg_renumber[r1no];
          if (r1no < 0 || r1no > LAST_HARD_REG)
            return false;
        }
    }
  if (r2 == NULL_RTX)
    return REGNO_MODE_OK_FOR_BASE_P (r1no, mode) || r1ok;
  int r2no = REGNO (r2);
  bool r2ok = true;
  if (strict)
    {
      r2ok = false;
      if (r2no > LAST_HARD_REG && reg_renumber != 0)
        {
          r2no = reg_renumber[r2no];
          if (r2no < 0 || r2no > LAST_HARD_REG)
            return false;
        }
    }
  if ((REGNO_OK_FOR_INDEX_P (r1no) || r1ok)
      && (REGNO_MODE_OK_FOR_REG_BASE_P (r2no, mode) || r2ok))
    return true;
  return (REGNO_OK_FOR_INDEX_P (r2no) || r2ok)
    && (REGNO_MODE_OK_FOR_REG_BASE_P (r1no, mode) || r1ok);
}

#undef  TARGET_ADDR_SPACE_ZERO_ADDRESS_VALID
#define TARGET_ADDR_SPACE_ZERO_ADDRESS_VALID ia16_as_zero_address_valid

static bool
ia16_as_zero_address_valid (addr_space_t addrspace)
{
  switch (addrspace)
    {
    case ADDR_SPACE_GENERIC:
      return false;
    case ADDR_SPACE_FAR:
      return true;
    default:
      gcc_unreachable ();
    }
}

static rtx
ia16_bless_selector (rtx seg)
{
  if (TARGET_PROTECTED_MODE && GET_MODE (seg) != PHImode)
    seg = force_reg (PHImode, gen_rtx_TRUNCATE (PHImode, seg));
  return seg;
}

static rtx
ia16_seg_override_term (rtx seg)
{
  return gen_seg_override_prot_mode (ia16_bless_selector (seg));
}

#define SEGmode		(TARGET_PROTECTED_MODE ? PHImode : HImode)

static void
ia16_error_seg_reloc (location_t loc, const char *msg)
{
  if (msg)
    error_at (loc, "%s", msg);
  inform (loc, "use %<-m%s%> to allow this",
	  TARGET_CMODEL_IS_TINY ? "cmodel=small" : "segment-relocation-stuff");
}

rtx
ia16_as_convert_weird_memory_address (machine_mode to_mode, rtx x,
				      addr_space_t as,
				      bool in_const ATTRIBUTE_UNUSED,
				      bool no_emit)
{
  rtx r1, r2, c, off, r9;

  gcc_assert (as == ADDR_SPACE_FAR);

  switch (to_mode)
  {
  case HImode:
    /* We want a far address, while x is possibly a far pointer.

       A far _pointer_ will likely appear in the form

	(... (reg/f:SI <i>) ...)

       i.e. an expression involving a register _pair_.  To form a far
       _address_, split this up into its segment and offset components, and
       pretend that the result is actually just a HImode operand, like so:

	(set (reg:HI <o>) (subreg:HI (reg/f:SI <i>) 0))
	(set (reg:HI <s>) (subreg:HI (reg/f:SI <i>) 2))
	(... (plus:HI (unspec:HI [(reg:SEG <s>)] UNSPEC_SEG_OVERRIDE)
		      (reg:HI <o>)) ...)

       This is (partially) in accord with gcc/rtl.h (see a comment at struct
       address_info), which says that `(unspec ...)' is typically used to
       represent segments.  */

    if (ia16_have_seg_override_p (x))
      return x;

    if (no_emit)
      return NULL_RTX;

    gcc_assert (GET_MODE (x) == SImode || GET_MODE (x) == VOIDmode);

    x = force_reg (SImode, x);
    r1 = force_reg (HImode, gen_rtx_SUBREG (HImode, x, 0));
    r9 = force_reg (SEGmode, gen_rtx_SUBREG (SEGmode, x, 2));

    x = gen_rtx_PLUS (HImode, r1, ia16_seg_override_term (r9));
    return x;

  case SImode:
    /* We want a far pointer, while x is possibly a far address.  */
    if (! ia16_parse_address (x, &r1, &r2, &c, &r9))
      return x;

    if (no_emit)
      return NULL_RTX;

    if (! r9)
      {
	switch (GET_CODE (x))
	  {
	  case CONST_INT:
	    /* Absolute far address --- nothing to do.  */
	    return x;

	  case SYMBOL_REF:
	    /* Address of a far static variable.  */
	    if (! TARGET_SEG_RELOC_STUFF)
	      {
		ia16_error_seg_reloc (input_location,
				      "cannot take address of far static "
				      "variable");
		/* continue after that... */
	      }

	    /* If CFUN is NULL, we are building a static initializer
	       containing the address of a far static variable.  */
	    if (! cfun)
	      return gen_static_far_ptr (x);

	    /* Otherwise, create a pseudo-register the normal way for the
	       segment term.  */
	    r9 = gen_seg16_reloc (x);
	    break;

	  default:
	    fprintf (stderr, "No idea how to convert this far address/pointer "
			     "to an SImode:\n");
	    debug_rtx (x);
	    gcc_unreachable ();
	  }
      }

    off = NULL_RTX;
    if (r1)
      off = r2 ? gen_rtx_PLUS (HImode, r1, r2) : r1;
    if (c)
      off = off ? gen_rtx_PLUS (HImode, x, c) : c;

    if (!off)
      off = const0_rtx;

    x = gen_reg_rtx (SImode);
    emit_move_insn (gen_rtx_SUBREG (HImode, x, 0), off);
    emit_move_insn (gen_rtx_SUBREG (GET_MODE (r9), x, 2), r9);
    return x;

  default:
    fprintf (stderr, "Trying to convert this far address/pointer to an "
		     "unexpected mode %u:\n", (unsigned) to_mode);
    debug_rtx (x);
    gcc_unreachable ();
  }
}

rtx
ia16_expand_weird_pointer_plus_expr (rtx op0, rtx op1, rtx target,
				     machine_mode mode)
{
  rtx seg, op0_off, off, sum;

  if (mode != SImode && mode != VOIDmode)
    {
      fprintf (stderr, "Trying to do far pointer addition with unexpected "
		       "target mode %u:\n", (unsigned) mode);
      debug_rtx (op0);
      debug_rtx (op1);
      gcc_unreachable ();
    }

  gcc_assert (GET_MODE (op0) == SImode || GET_MODE (op0) == VOIDmode);
  gcc_assert (GET_MODE (op1) == HImode || GET_MODE (op1) == VOIDmode);

  if (GET_CODE (op0) == CONST
      && GET_CODE (XEXP (op0, 0)) == UNSPEC
      && XINT (XEXP (op0, 0), 1) == UNSPEC_STATIC_FAR_PTR)
    {
      /* This pointer arithmetic operation occurs within a static
	 initializer.  */
      op0 = XVECEXP (XEXP (op0, 0), 0, 0);
      return gen_static_far_ptr (gen_rtx_PLUS (GET_MODE (op0), op0, op1));
    }

  op0 = force_reg (SImode, op0);

  seg = gen_rtx_SUBREG (SEGmode, op0, 2);
  op0_off = gen_rtx_SUBREG (HImode, op0, 0);

  off = force_reg (HImode, gen_rtx_PLUS (HImode, op0_off, op1));

  if (target && REG_P (target) && GET_MODE (target) == SImode)
    sum = target;
  else
    sum = gen_reg_rtx (SImode);

  emit_move_insn (gen_rtx_SUBREG (HImode, sum, 0), off);
  emit_move_insn (gen_rtx_SUBREG (SEGmode, sum, 2), seg);
  return sum;
}

/* Used by ia16_as_legitimize_address, and by ia16.md.  */
void
ia16_split_seg_override_and_offset (rtx x, rtx *ovr, rtx *off)
{
  machine_mode m = GET_MODE (x);
  rtx op0, op1;
  rtx op0_ovr, op0_off, op1_ovr, op1_off;

  switch (GET_CODE (x))
    {
    case UNSPEC:
      if (XINT (x, 1) == UNSPEC_SEG_OVERRIDE)
	{
	  *ovr = x;
	  *off = NULL_RTX;
	  break;
	}
      /* fall through */
    default:
      *ovr = NULL_RTX;
      *off = x;
      break;

    case PLUS:
      op0 = XEXP (x, 0);
      op1 = XEXP (x, 1);

      ia16_split_seg_override_and_offset (op0, &op0_ovr, &op0_off);
      ia16_split_seg_override_and_offset (op1, &op1_ovr, &op1_off);

      gcc_assert (! op0_ovr || ! op1_ovr);
      *ovr = op0_ovr ? op0_ovr : op1_ovr;

      if (! op0_off)
	*off = op1_off;
      else if (! op1_off)
	*off = op0_off;
      else if (op0 == op0_off && op1 == op1_off)
	*off = x;
      else
	*off = gen_rtx_PLUS (m, op0_off, op1_off);
      break;

    case MINUS:
      op0 = XEXP (x, 0);
      op1 = XEXP (x, 1);

      ia16_split_seg_override_and_offset (op0, &op0_ovr, &op0_off);
      *ovr = op0_ovr;
      if (! op0_off)
	*off = gen_rtx_NEG (m, op1);
      else
	*off = gen_rtx_MINUS (m, op0_off, op1);
    }
}

#undef	TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS
#define	TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS ia16_as_legitimize_address

static rtx
ia16_as_legitimize_address (rtx x, rtx oldx,
			    machine_mode mode ATTRIBUTE_UNUSED,
			    addr_space_t as ATTRIBUTE_UNUSED)
{
  rtx ovr, off, newx;

  if (as == ADDR_SPACE_GENERIC
      || ia16_as_legitimate_address_p (mode, x, false, as))
    return x;

  /* We must be able to transform an expression like
   *
   *	                       (plus:HI)
   *	                      /         \
   *	              (plus:HI)         (mem:HI)
   *	                /  \               |
   *	(unspec:HI [.] 1)  (reg:HI 25)  (plus:HI)
   *	            |                     /  \
   *	      (reg:HI 26)       (reg:HI 15)  (const_int 4)
   *
   * (q.v. https://github.com/tkchia/gcc-ia16/issues/6) into something
   * resembling an IA-16 addressing mode.
   *
   * Because of the segment override term (unspec:HI [.] 1), we cannot
   * simply force-fit the whole thing into a (reg:HI) with force_reg (...)
   * and call it a day.
   *
   * What we can do is to split the expression into its segment override and
   * offset components.  Then we _can_ try to force the offset component
   * into a single (reg:HI), if needed.
   */
  gcc_assert (GET_MODE (x) == HImode);

  ia16_split_seg_override_and_offset (x, &ovr, &off);

  if (! off)
    off = const0_rtx;

  /* This lack of a segment override may mean one of two things.

     One possibility is that we want to take the address of a far static
     variable.

     Alternatively, it may be due to GCC's internal probing ---
     specifically, tree-ssa-loop-ivopts.c creates nonsense addresses in
     order to gauge the costs of using different addressing modes.

     This code needs to work in both cases.  */
  if (! ovr)
    ovr = ia16_seg_override_term (force_reg (HImode, gen_seg16_reloc (oldx)));

  newx = gen_rtx_PLUS (HImode, off, ovr);
  if (ia16_as_legitimate_address_p (mode, newx, false, as))
    return newx;

  return gen_rtx_PLUS (HImode, force_reg (HImode, off), ovr);
}

#undef	TARGET_ADDR_SPACE_SUBSET_P
#define	TARGET_ADDR_SPACE_SUBSET_P ia16_as_subset_p

static bool
ia16_as_subset_p (addr_space_t subset, addr_space_t superset)
{
  return subset == ADDR_SPACE_GENERIC && superset == ADDR_SPACE_FAR;
}

static rtx
ia16_far_pointer_offset (rtx op)
{
  /* Use (subreg ...) if we can.  Otherwise use (truncate ...).  */
  if (REG_P (op))
    return gen_rtx_SUBREG (HImode, op, 0);
  else
    return gen_rtx_TRUNCATE (HImode, op);
}

#undef	TARGET_ADDR_SPACE_CONVERT
#define	TARGET_ADDR_SPACE_CONVERT ia16_as_convert

static rtx
ia16_as_convert (rtx op, tree from_type, tree to_type)
{
  gcc_assert (POINTER_TYPE_P (from_type));
  gcc_assert (POINTER_TYPE_P (to_type));

  from_type = TREE_TYPE (from_type);
  to_type = TREE_TYPE (to_type);

  if (TYPE_ADDR_SPACE (from_type) == ADDR_SPACE_FAR
      && TYPE_ADDR_SPACE (to_type) == ADDR_SPACE_GENERIC)
    {
      /* We only handle pointers for now --- not addresses.  */
      gcc_assert (GET_MODE (op) == SImode || GET_MODE (op) == VOIDmode);

      return ia16_far_pointer_offset (op);
    }
  else if (TYPE_ADDR_SPACE (from_type) == ADDR_SPACE_GENERIC
	   && TYPE_ADDR_SPACE (to_type) == ADDR_SPACE_FAR)
    {
      rtx op2 = gen_reg_rtx (SImode);
      gcc_assert (GET_MODE (op) == HImode || GET_MODE (op) == VOIDmode);

      emit_move_insn (gen_rtx_SUBREG (HImode, op2, 0), op);
      if (FUNC_OR_METHOD_TYPE_P (from_type))
	emit_move_insn (gen_rtx_SUBREG (HImode, op2, 2),
			gen_rtx_REG (HImode, CS_REG));
      else
	emit_move_insn (gen_rtx_SUBREG (HImode, op2, 2),
			gen_rtx_REG (HImode, SS_REG));

      return op2;
    }
  else
    gcc_unreachable ();
}

/* Condition Code Status */
/* Return the "smallest" usable comparison mode for the given comparison
 * operator OP and operands X and Y.  BRANCH is true if we are optimizing for
 * a branch instruction.  */
enum machine_mode
ia16_select_cc_mode (enum rtx_code op, rtx x, rtx y,
		     bool branch ATTRIBUTE_UNUSED)
{
  switch (op)
    {
    case EQ:
    case NE:
      /* TODO: Explain why we check nonimmediate_operand(x) here.  */
      if ((y == const0_rtx || y == constm1_rtx)
	   && nonimmediate_operand (x, GET_MODE (x)))
	return y == const0_rtx ? CCZ_Cmode : CCZ_NCmode;
      else
	return CCZmode;

    case LT:
    case GE:
      /* We don't need to set the overflow flag when comparing against zero.
	 The add/sub instructions don't set the overflow flag usefully.  */
      if (y == const0_rtx)
	return CCSmode;
      else
	return CCSOmode;

    case LE:
    case GT:
      /* We have no jump instructions for CCSZmode, so always use CCSOZmode.  */
      return CCSOZmode;

    case LTU:
    case GEU:
      return CCCmode;

    case LEU:
    case GTU:
      /* Detect overflow checks.  */
      if (GET_CODE (x) == MINUS
	  && rtx_equal_p (XEXP (x, 0), y))
	return CCCZ_NCmode;
      /* Comparison against immediates don't need to set the zero flag.  */
      return immediate_operand (y, GET_MODE (y)) ? CCCZ_Cmode : CCCZmode;

    default:
    return CCmode;
    }
}

/* Emit a comparison instruction for the comparison operator OP and the two
 * operands X and Y. If BRANCH is true, optimize for a branch instruction.
 * Return the register which holds the comparison result.  */
rtx
ia16_gen_compare_reg (enum rtx_code op, rtx x, rtx y, bool branch)
{
  enum machine_mode mode = ia16_select_cc_mode (op, x, y, branch);
  rtx cc_reg = gen_rtx_REG (mode, CC_REG);

  emit_insn (gen_rtx_SET (cc_reg, gen_rtx_COMPARE (mode, x, y)));
  return cc_reg;
}

#undef  TARGET_CANONICALIZE_COMPARISON
#define TARGET_CANONICALIZE_COMPARISON ia16_canonicalize_comparison

static void
ia16_canonicalize_comparison (int *code, rtx *op0, rtx *op1,
			      bool op0_preserve_value ATTRIBUTE_UNUSED)
{
  if ((*code == EQ || *code == NE) && GET_CODE (*op1) == NEG)
    {
      *op0 = gen_rtx_PLUS (GET_MODE (*op0), *op0, XEXP (*op1, 0));
      *op1 = const0_rtx;
    }
}

#undef  TARGET_FIXED_CONDITION_CODE_REGS
#define TARGET_FIXED_CONDITION_CODE_REGS ia16_fixed_condition_code_regs

/* CC_REG is a fixed condition code register.  */
static bool
ia16_fixed_condition_code_regs (unsigned int *reg1, unsigned int *reg2)
{
	*reg1 = CC_REG;
	*reg2 = INVALID_REGNUM;
	return (true);
}

#undef  TARGET_CC_MODES_COMPATIBLE
#define TARGET_CC_MODES_COMPATIBLE ia16_cc_modes_compatible

/* TODO: Convert this into a table.  */
enum machine_mode
ia16_cc_modes_compatible (enum machine_mode mode1, enum machine_mode mode2)
{
  switch (mode1)
    {

      case CCSCmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	      return (CCSCZmode);
	    case CCSCmode:
	    case CCCmode:
	      return (CCSCmode);
	    default:
	      return (CCmode);
	  }

      case CCSOZmode:
	switch (mode2)
	  {
	    case CCSOZmode:
	    case CCSOmode:
	    case CCSZmode:
	    case CCZmode:
	      return (CCSOZmode);
	    default:
	      return (CCmode);
	  }

      case CCSOmode:
	switch (mode2)
	  {
	    case CCSOZmode:
	    case CCSOmode:
	      return (mode2);
	    case CCSmode:
	      return (CCSOmode);
	    default:
	      return (CCmode);
	  }

      case CCSmode:
	switch (mode2)
	  {
	    case CCSZmode:
	    case CCSOmode:
	    case CCSOZmode:
	      return (mode2);
	    default:
	      return (CCmode);
	  }

      case CCCZmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	      return (CCSCZmode);
	    case CCCZmode:
	    case CCZmode:
	    case CCCmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCZmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	    case CCSOZmode:
	    case CCSZmode:
	    case CCCZmode:
	    case CCZmode:
	      return (mode2);
	    case CCSOmode:
	      return (CCSOZmode);
	    case CCCmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCCmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	    case CCSCmode:
	    case CCCZmode:
	    case CCCmode:
	      return (mode2);
	    case CCZmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCmode:
      default:
	return (CCmode);
    }
    gcc_unreachable ();
}

/* Describing Relative Costs of Operations */

/* This is modelled after the i386 port.
   Often, there are different costs reg,reg->reg, mem,reg->reg
   and reg,mem->mem.  The cost of memory address calculation, calculated using
   ea_calc (also for lea), will be added except for xlat.
*/
struct processor_costs {
  const int byte_fetch;		/* cost of fetching an instruction byte.  */
  int (*const ea_calc)(rtx r1, rtx r2, rtx c, rtx r9);
				/* cost of calculating an address.  */
  const int move;		/* cost of reg-reg mov instruction.  */
  const int imm_load[2];	/* cost of loading imm (QI, HI) to register.  */
  const int imm_store[2];	/* cost of storing imm (QI, HI) to memory.  */
  const int int_load[2];	/* cost of loading integer register
				   from memory in QImode and HImode.  */
  const int int_store[2];	/* cost of storing integer register
				   to memory in QImode and HImode */
  const int move_ratio;		/* The threshold of number of scalar
				   memory-to-memory move insns.  */
  const int add[3];		/* cost of an add instruction */
  const int add_imm[2];		/* cost of an add imm (reg, mem) instruction */
  const int inc_dec[2];
  const int lea;		/* cost of a lea instruction */
  const int cmp[3];		/* cost of a cmp instruction */
  const int cmp_imm[2];		/* cost of a cmp imm (reg, mem) instruction */
  const int shift_1bit[2];	/* single bit shift/rotate cost */
  const int shift_start[2];	/* cost of starting shift/rotate (reg, mem) */
  const int shift_bit;		/* cost of shift/rotate per bit */
  const int s_mult_init[2][2];	/* cost of starting a signed multiply in
				   { QI { r, m }, HI { r, m } } */
  const int u_mult_init[2][2];	/* cost of starting an unsigned multiply in
				   { QI { r, m }, HI { r, m } } */
  const int mult_imm_init[2];	/* cost of starting imul reg/mem.  */
  const int mult_bit;		/* cost of multiply per each bit set */
  const int s_divide[2][2];	/* cost of a signed divide/mod
				   in QImode, HImode */
  const int u_divide[2][2];	/* cost of an unsigned divide.  */
  const int sign_extend[2];	/* The cost of sign extension (QI, HI).  */
  const int xlat;		/* cost of an xlat instruction.  */
  const int call[3];		/* cost of call {imm, reg, mem}.  */
  const int fp_move;		/* cost of reg,reg fld/fst */
  const int fp_load[3];		/* cost of loading FP register
				   in SFmode, DFmode and XFmode */
  const int fp_store[3];	/* cost of storing FP register
				   in SFmode, DFmode and XFmode */
  const int branch_cost;	/* Default value for BRANCH_COST.  */
  const int fadd;		/* cost of FADD and FSUB instructions.  */
  const int fmul;		/* cost of FMUL instruction.  */
  const int fdiv;		/* cost of FDIV instruction.  */
  const int fabs;		/* cost of FABS instruction.  */
  const int fchs;		/* cost of FCHS instruction.  */
  const int fsqrt;		/* cost of FSQRT instruction.  */
};

extern const  struct processor_costs *ia16_costs;
extern struct processor_costs ia16_size_costs;

#define IA16_COST(F)	\
	(MAX (ia16_costs->F, ia16_costs->byte_fetch * ia16_size_costs.F))

#define C(x)	COSTS_N_INSNS(x)

/* Return the cost of a constant X in mode MODE in an OUTER_CODE rtx.  */
/* TODO: Accurate costs of vector constants.  */
static int
ia16_constant_cost (rtx x, enum machine_mode mode, int outer_code)
{
  int n;

  if (!CONST_INT_P (x))
    return (C (ia16_costs->byte_fetch * GET_MODE_SIZE (Pmode)));

  n = INTVAL (x);
  if (mode == VOIDmode)
    mode = Pmode;

  /* Some instructions have implicit constants.  */
  if (n == 1)
    switch (outer_code)
      {
	case PLUS:
	case MINUS:
	case ASHIFT:
	case ASHIFTRT:
	case LSHIFTRT:
	case ROTATE:
	case ROTATERT:
	case EQ:
	case NE:
	case LTU:
	case GEU:
	case LEU:
	case GTU:
	case GT:
	case LT:
	case LE:
	case GE:
	  return (C (0));

	default:
	  break;
      }
  if (n == 0)
    switch (outer_code)
      {
	case MINUS:
	case EQ:
	case NE:
	case LTU:
	case GEU:
	case LEU:
	case GTU:
	case GT:
	case LT:
	case LE:
	case GE:
	  return (C (0));

	default:
	  break;
      }
  if ((outer_code == PLUS && n == -1)
      || ((outer_code == AND || outer_code == XOR)
	 && (n == 255 || n == -256)))
    return (C (0));

  /* Most instructions can sign extend 8-bit constants.  Exceptions:
   * "movw $imm, reg", "testw $imm, dest" and "enterw $imm, $level".
   * We catch the "movw" case here.  We try to peephole "testw" away.
   * Some insn alternatives for "add", "andw", "orw" and "xorw" hack
   * zero extension of 8-bit constants.
   */
  if ((outer_code != SET && n >= -128 && n <= 127)
      || (outer_code == AND
	 && ((n & 0xff00) == 0xff00 || (n & 0x00ff) == 0x00ff))
      || ((outer_code == IOR || outer_code == XOR)
	 && ((n & 0xff00) == 0x0000 || (n & 0xff00) == 0x0000))
      || (outer_code == PLUS && (n & 0x00ff) == 0x0000))
    return (C (ia16_costs->byte_fetch));
  else
    return (C (ia16_costs->byte_fetch * GET_MODE_SIZE (mode)));
}

/* Return the cost of an address when not optimizing for an Intel i808x.
 */
static int
ia16_default_address_cost (rtx r1, rtx r2, rtx c, rtx r9 ATTRIBUTE_UNUSED)
{
  int total = 0;

  if (c)
    total = ia16_constant_cost (c, Pmode, MEM);

  /* If r1 == r2, a "movw" instruction is needed.  */
  if (rtx_equal_p (r1, r2))
     total += IA16_COST (move);

  return (total);
}

/* Calculate address cost for Intel i8086/i8088.  Note that %bp + %si or
 * %bx + %di costs one more cycle than %bp + %di or %bx + %si.
 * The %bp addressing mode is really 0 + %bp but we ignore that.
 */
static int
ia16_i808x_address_cost (rtx r1, rtx r2, rtx c, rtx r9)
{
  int cost = 0;

  if (r1)
    {
      if (r2)
        {
          cost = (C (7) + C (8)) / 2;

          /* If r1 == r2, a "movw" instruction is needed.  */
          if (rtx_equal_p (r1, r2))
            cost += IA16_COST (move);
        }
      else
        cost = C (5);

      if (c)
        cost += C (4) + ia16_constant_cost (c, Pmode, MEM);
    }
  else
    {
      if (c)
	cost = C (6) + ia16_constant_cost (c, Pmode, MEM);
    }

  if (r9)
    cost += C (2);

  return (cost);
}

/* Return the cost of an address when optimizing for size rather than speed. */
static int
ia16_size_address_cost (rtx r1, rtx r2, rtx c, rtx r9)
{
  /* Treat the r/m portion of a mod-r/m encoding as taking up half a byte.  */
  int cost = (C (0) + C (1)) / 2;

  if (r1)
    {
      if (r2)
        {
          /* If r1 == r2, a "movw" instruction is needed.  */
          if (rtx_equal_p (r1, r2))
            cost += IA16_COST (move);
        }
    }

  if (c)
    cost += ia16_constant_cost (c, Pmode, MEM);

  if (r9)
    cost += C (1);

  return (cost);
}

/* Size costs for IA-16 instructions.  Used when optimizing for size.
 * EA sizes are not included except for xlat.
 */
 struct processor_costs ia16_size_costs = {
  /* byte_fetch */	1,
  /* ea_calc */		ia16_size_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (1), C (1) },
  /* imm_store */	{ C (2), C (2) },
  /* int_load */	{ C (2), C (2) },
  /* int_store */	{ C (2), C (2) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (2), C (2) },
  /* add_imm */		{ C (2), C (2) },
  /* inc_dec */		{ C (1), C (2) },
  /* lea */		C (2),
  /* cmp */		{ C (2), C (2), C (2) },
  /* cmp_imm */		{ C (2), C (2) },
  /* shift_1bit	*/	{ C (2), C (2) },
  /* shift_start */	{ C (2), C (2) },
  /* shift_bit */	C (0),
  /* s_mult_init */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* u_mult_init */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* mult_imm_init */	{ C (2), C (2) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* u_divide */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* sign_extend */	{ C (1), C (1) },
  /* xlat */		C (1),
  /* call */		{ C (1), C (1), C (1) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel i8086 CPUs.  EA calculation time is not included.
 * Timings have been derived from the Intel i8088 timings.
 * TODO: .mult_bit is closer to 2 for signed multiplies.
 */
static struct processor_costs ia16_i8086_costs = {
  /* byte_fetch	*/	2,
  /* ea_calc */		ia16_i808x_address_cost,
			/* Nasty hack to prevent spurious reloads and spills
			   when compiling with -O3 --- see the description at
			   https://github.com/tkchia/gcc-ia16/issues/15 .
			   The value here is currently the maximum of
			   .int_load[1] and .int_store[1].  A more proper
			   patch is needed.  -- tkchia 20171224  */
  /* move */		C (9) /* C (2) */,
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (10), C (10) },
  /* int_load */	{ C (8), C (8) },
  /* int_store */	{ C (9), C (9) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (9), C (16) },
  /* add_imm */		{ C (4), C (17) },
  /* inc_dec */		{ C (3), C (16) },
  /* lea */		C (2),
  /* cmp */		{ C (3), C (9), C (9) },
  /* cmp_imm */		{ C (4), C (10) },
  /* shift_1bit */	{ C (2), C (15) },
  /* shift_start */	{ C (8), C (20) },
  /* shift_bit */	C (4),
  /* s_mult_init */	{ { C (80), C (86) }, { C (128), C (134) } },
  /* u_mult_init */	{ { C (70), C (76) }, { C (118), C (124) } },
  /* mult_imm_init */	{ C (132), C (138) },
  /* mult_bit */	C (1),
  /* s_divide */	{ { C (106), C (112) }, { C (174), C (180) } },
  /* u_divide */	{ { C (85), C (91) }, { C (153), C (159) } },
  /* sign_extend */	{ C (2), C (5) },
  /* xlat */		C (11),
  /* call */		{ C (19), C (16), C (21) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 8088 CPUs.  EA calculation time is not included.  */
/* TODO: .mult_bit is closer to 2 for signed multiplies.  */
static struct processor_costs ia16_i8088_costs = {
  /* byte_fetch */	4,
  /* ea_calc */		ia16_i808x_address_cost,
  /* move */		C (13) /* C (2) */,
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (10), C (14) },
  /* int_load */	{ C (8), C (12) },
  /* int_store */	{ C (9), C (13) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (13), C (24) },
  /* add_imm */		{ C (4), C (25) },
  /* inc_dec */		{ C (3), C (24) },
  /* lea */		C (2),
  /* cmp */		{ C (3), C (13), C (13) },
  /* cmp_imm */		{ C (4), C (14) },
  /* shift_1bit */	{ C (2), C (23) },
  /* shift_start */	{ C (8), C (28) },
  /* shift_bit */	C (4),
  /* s_mult_init */	{ { C (80), C (86) }, { C (128), C (138) } },
  /* u_mult_init */	{ { C (70), C (76) }, { C (118), C (128) } },
  /* mult_imm_init */	{ C (132), C (142) },
  /* mult_bit */	C (1),
  /* s_divide */	{ { C (106), C (112) }, { C (174), C (184) } },
  /* u_divide */	{ { C (85), C (91) }, { C (153), C (163) } },
  /* sign_extend */	{ C (2), C (5) },
  /* xlat */		C (11),
  /* call */		{ C (23), C (20), C (29) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 80186 CPUs.  EA calculation time (4 cycles for all modes)
 * is included wherever appropriate.  */
/* TODO: Intel 80188 timings.  */
static struct processor_costs ia16_i80186_costs = {
  /* byte_fetch	*/	2,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (12) /* C (2) */,
  /* imm_load */	{ C (3), C (4) },
  /* imm_store */	{ C (12), C (13) },
  /* int_load */	{ C (9), C (9) },
  /* int_store */	{ C (12), C (12) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (10), C (15) },
  /* add_imm */		{ C (4), C (16) },
  /* inc_dec */		{ C (3), C (15) },
  /* lea */		C (6),
  /* cmp */		{ C (3), C (10), C (10) },
  /* cmp_imm */		{ C (3), C (10) },
  /* shift_1bit */	{ C (2), C (15) },
  /* shift_start */	{ C (5), C (17) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (26), C (32) }, { C (35), C (41) } },
  /* u_mult_init */	{ { C (27), C (33) }, { C (36), C (42) } },
  /* mult_imm_init */	{ C (23), C (30) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (48), C (54) }, { C (57), C (63) } },
  /* u_divide */	{ { C (29), C (35) }, { C (38), C (44) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (11),
  /* call */		{ C (15), C (13), C (19) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 80286 CPUs.  EA calculation time (? cycles for all modes)
 * is included wherever appropriate.  */
/* TODO: Check timings for signed vs. unsigned multiply.  */
static struct processor_costs ia16_i80286_costs = {
  /* byte_fetch */	1, /* guess */
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (2), C (2) },
  /* imm_store */	{ C (3), C (3) },
  /* int_load */	{ C (5), C (5) },
  /* int_store */	{ C (3), C (3) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (7), C (7) },
  /* add_imm */		{ C (3), C (7) },
  /* inc_dec */		{ C (2), C (7) },
  /* lea */		C (3),
  /* cmp */		{ C (3), C (6), C (7) },
  /* cmp_imm */		{ C (3), C (6) },
  /* shift_1bit */	{ C (2), C (7) },
  /* shift_start */	{ C (5), C (8) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (13), C (16) }, { C (21), C (24) } },
  /* u_mult_init */	{ { C (13), C (16) }, { C (21), C (24) } },
  /* mult_imm_init */	{ C (21), C (24) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (17), C (20) }, { C (25), C (28) } },
  /* u_divide */	{ { C (14), C (17) }, { C (22), C (25) } },
  /* sign_extend */	{ C (2), C (2) },
  /* xlat */		C (5),
  /* call */		{ C (7), C (7), C (11) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for NEC V30 CPUs.  EA calculation time (2 cycles for all modes)
 * is included wherever appropriate.  Timings are derived from the V20 ones.
 */
static struct processor_costs ia16_nec_v30_costs = {
  /* byte_fetch */	2,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (11) /* C (2) */,
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (11), C (11) },
  /* int_load */	{ C (11), C (11) },
  /* int_store */	{ C (9), C (9) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (11), C (16) },
  /* add_imm */		{ C (4), C (18) },
  /* inc_dec */		{ C (2), C (16) },
  /* lea */		C (4),
  /* cmp */		{ C (2), C (11), C (11) },
  /* cmp_imm */		{ C (4), C (13) },
  /* shift_1bit */	{ C (2), C (16) },
  /* shift_start */	{ C (7), C (19) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (33), C (39) }, { C (47), C (53) } },
  /* u_mult_init */	{ { C (21), C (30) }, { C (27), C (32) } },
  /* mult_imm_init */	{ C (31), C (34) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (29), C (35) }, { C (43), C (49) } },
  /* u_divide */	{ { C (19), C (25) }, { C (25), C (31) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (9),
  /* call */		{ C (16), C (14), C (23) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for NEC V20 CPUs.  EA calculation time (2 cycles for all modes)
 * is included wherever appropriate.  */
static struct processor_costs ia16_nec_v20_costs = {
  /* byte_fetch */	4,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (15) /* C (2) */,
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (11), C (15) },
  /* int_load */	{ C (11), C (15) },
  /* int_store */	{ C (9), C (13) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (15), C (24) },
  /* add_imm */		{ C (4), C (26) },
  /* inc_dec */		{ C (2), C (24) },
  /* lea */		C (4),
  /* cmp */		{ C (2), C (15), C (15) },
  /* cmp_imm */		{ C (4), C (17) },
  /* shift_1bit */	{ C (2), C (24) },
  /* shift_start */	{ C (7), C (27) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (33), C (39) }, { C (47), C (57) } },
  /* u_mult_init */	{ { C (21), C (30) }, { C (27), C (36) } },
  /* mult_imm_init */	{ C (31), C (38) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (29), C (35) }, { C (43), C (53) } },
  /* u_divide */	{ { C (19), C (25) }, { C (25), C (35) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (9),
  /* call */		{ C (20), C (18), C (31) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

#undef C

/* This table must be in sync with enum processor_type in ia16.h.  */
static const struct ptt processor_target_table[PROCESSOR_max] =
{
  {&ia16_i8086_costs,    0},
  {&ia16_i8086_costs,    1},
  {&ia16_i8086_costs,    4},
  {&ia16_i8088_costs,   20},
  {&ia16_nec_v30_costs,  1},
  {&ia16_nec_v20_costs, 17},
  {&ia16_i80186_costs,   7},
  {&ia16_i80186_costs,  23},
  {&ia16_i80286_costs,  15}
};

const struct processor_costs *ia16_costs = &ia16_i8086_costs;
int ia16_features = 0;

#define I_REG	0
#define I_MEM	1
#define O_REGREG 0
#define O_MEMREG 1
#define O_REGMEM 2
#define M_QI	0
#define M_HI	1

#define I_RTX(x)	(MEM_P (x) ? I_MEM : I_REG)
#define M_MOD(x)	(QImode == (x) ? M_QI : M_HI)
#define M_RTX(x)	(QImode == GET_MODE (x) ? M_QI : M_HI)
#define H_MOD(x)	ia16_mode_hwords (x)
#define H_RTX(x)	ia16_mode_hwords (GET_MODE (x))

static unsigned
ia16_mode_hwords (machine_mode mode)
{
  return (GET_MODE_SIZE (mode) + 1) / 2;
}

#undef	TARGET_REGISTER_MOVE_COST
#define	TARGET_REGISTER_MOVE_COST ia16_register_move_cost

static int
ia16_register_move_cost (machine_mode mode, reg_class_t from ATTRIBUTE_UNUSED,
			 reg_class_t to ATTRIBUTE_UNUSED)
{
  return IA16_COST (move) * ia16_mode_hwords (mode);
}

#undef	TARGET_MEMORY_MOVE_COST
#define	TARGET_MEMORY_MOVE_COST ia16_memory_move_cost

/* It is, on average, slightly cheaper to copy to/from memory with al/ax
 * than with other registers, because a faster, immediate address
 * instruction exists.  Segments registers, if supported, would also be
 * cheaper to copy to/from memory.
 */
static int
ia16_memory_move_cost (machine_mode mode, reg_class_t rclass ATTRIBUTE_UNUSED,
		       bool in)
{
  int cost;

  if (in)
    {
      if (GET_MODE_SIZE (mode) == 1)
	cost = IA16_COST (int_load[M_QI]);
      else
	cost = IA16_COST (int_load[M_HI]) * ia16_mode_hwords (mode);
    }
  else
    {
      if (GET_MODE_SIZE (mode) == 1)
	cost = IA16_COST (int_store[M_QI]);
      else
	cost = IA16_COST (int_store[M_HI]) * ia16_mode_hwords (mode);
    }

  if (rclass == AX_REGS
      || rclass == SEGMENT_REGS
      || rclass == ES_REGS
      || rclass == DS_REGS)
    {
      if (cost > COSTS_N_INSNS (1))
	cost -= COSTS_N_INSNS (1);
      else
	cost = 1;
    }

  return cost;
}

#undef  TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST ia16_address_cost

static int
ia16_address_cost_internal (rtx address)
{
  rtx r1, r2, c, r9;

  /* Addresses in stack pushes/pops are cheap.  */
  if (PRE_MODIFY == GET_CODE (address)
      || POST_MODIFY == GET_CODE (address)
      || PRE_DEC == GET_CODE (address)
      || POST_INC == GET_CODE (address))
    return (0);

  /* Check for a (call ...) address.  This is a hack, but without knowing
   * the mode, this is the best we can do.  At least it won't crash.  */
  if (MEM_P (address))
    address = XEXP (address, 0);

  /* Parse the address.  */
  ia16_parse_address (address, &r1, &r2, &c, &r9);

  return IA16_COST (ea_calc (r1, r2, c, r9));
}

static int
ia16_address_cost (rtx address, machine_mode, addr_space_t, bool)
{
  return ia16_address_cost_internal(address);
}

/* Return true if X is a valid memory operand for xlat and return the cost
 * in TOTAL.  Don't touch TOTAL otherwise.
 * This is complicated because combine uses many different ways of expressing
 * xlat.  We support:
 * (mem:QI (plus:HI (zero_extend:HI (mem_subreg_reg:QI op1)) (base:HI))
 * (mem:QI (plus:HI (zero_extract:HI (mem_subreg_reg:HI op1) (8) (8)) (base:HI))
 * (mem:QI (plus:HI (and:HI (stuff:HI op1) (255)) (base:HI))
 * (mem:QI (plus:HI (lshiftrt:HI (stuff) (const_int fubar)) (base:HI)))
 * and strip subregs as necessary.
 */
static bool
ia16_xlat_cost (rtx x, int *total)
{
      if (GET_MODE (x) != QImode)
        return (false);
      if (GET_CODE (XEXP (x, 0)) == PLUS)
	{
	  rtx plus, zext, op1, base;

	  plus = XEXP (x, 0);
	  zext = XEXP (plus, 0);
	  base = XEXP (plus, 1);
	  if (GET_CODE (zext) == SUBREG)
	    zext = SUBREG_REG (zext);
	  if (((GET_CODE (zext) != ZERO_EXTEND && GET_CODE (zext) != ZERO_EXTRACT
	        && (GET_CODE (zext) != AND || !CONST_INT_P (XEXP (zext, 1))
	            || INTVAL (XEXP (zext, 1)) != 255))
	        && (GET_CODE (zext) != LSHIFTRT || !CONST_INT_P (XEXP (zext, 1))))
	      || (!general_operand (base, Pmode)))
	    return (false);
	
	  op1 = XEXP (zext, 0);
/*	  op2 = XEXP (zext, 1); */
	  if (GET_CODE (op1) == SUBREG)
	    op1 = SUBREG_REG (op1);
	  if (!REG_P (op1) && !MEM_P (op1)
	      && (GET_CODE (op1) != LSHIFTRT /*|| !CONST_INT_P (op2)*/))
	    return (false);
	  *total = IA16_COST (xlat);

	  /* We may need to load the extendee into a register.  */
	  if (!REG_P (op1))
	    *total += IA16_COST (int_load[M_QI]);

	  /* We may need to load the base into a register.  */
	  if (CONSTANT_P (base))
	    *total += IA16_COST (imm_load[M_HI]);
	  else if (MEM_P (base))
	    *total += IA16_COST (int_load[M_HI])
		      + ia16_address_cost_internal (XEXP (base, 0));
	  return (true);	
	}
      return (false);
}

#undef  TARGET_RTX_COSTS
#define TARGET_RTX_COSTS	ia16_rtx_costs

/* Compute a (partial) cost for rtx X.  Return true if the complete
   cost has been computed, and false if subexpressions should be
   scanned.  In either case, *TOTAL contains the cost result.
   FIXME: Most costs are too low for modes larger than HImode.
   FIXME: When called from combine, we only ever see the source of SET
   expressions. We don't want to return a higher cost for
	(set (mem) (and (mem) (reg1))
   than we do for
	(set (reg2) (mem))
	(set (reg2) (and (reg2) (reg1))
	(set (mem) (reg2))
   so don't count the MEM twice if we get only the source expression.
*/

static bool
ia16_rtx_costs (rtx x, machine_mode mode, int outer_code_i,
		int opno ATTRIBUTE_UNUSED, int *total, bool speed)
{
  enum rtx_code code = GET_CODE (x);
  enum rtx_code outer_code = (enum rtx_code) outer_code_i;

  switch (code)
    {
    case SET:
      {
	rtx op0 = XEXP (x, 0);
	rtx op1 = XEXP (x, 1);

	if (memory_operand (op0, GET_MODE (op0)))
	  {
	    if (register_operand (op1, GET_MODE (op1)))
	      *total = IA16_COST (int_store[M_RTX (op0)]) * H_RTX (op0);
	    else if (immediate_operand (op1, GET_MODE (op1)))
	      *total = IA16_COST (imm_store[M_RTX (op0)]) * H_RTX (op0);
	    else
	      {
#if 0
		/* Arithmetic or logic instruction which writes to memory.
		 * Subtract the mem,reg->reg cost which will incorrectly be
		 * added in the MEM case below.  */
		*total = IA16_COST (add[O_REGMEM]) - IA16_COST (add[O_MEMREG]);
#else
		do {} while (0);
#endif
	      }
	  }
	else
	  {
	    if (register_operand (op0, GET_MODE (op1)))
	      {
		if (register_operand (op1, GET_MODE (op1)))
		  *total = IA16_COST (move);
		else if (memory_operand (op1, GET_MODE (op1)))
		  *total = IA16_COST (int_load[M_RTX (op1)]);
		else if (immediate_operand (op1, GET_MODE (op1)))
		  *total = IA16_COST (imm_load[M_RTX (op0)]);
		*total *= H_RTX (op0);
	      }
	    /* Try to get constructs involving "xlat" right.  */
	    else
	      {
		if (MEM_P (op1) && ia16_xlat_cost (op1, total))
		  return (true);
	      }
	  }
	return (false);
      }

    /* It is not easy to give an accurate cost of a memory access.  "movw" is
     * handled above.  Return a reasonable estimate for arithmetic or logic
     * instructions.  */
    case MEM:
      if ((outer_code == ZERO_EXTEND || outer_code == SET)
	&& ia16_xlat_cost (x, total))
	return (true);
      *total = ia16_address_cost_internal (XEXP (x, 0));
      /* This is never called recursively from the SET case.  */
      if (outer_code == SET)
	*total += IA16_COST (int_load[M_MOD (mode)]);
#if 0 /* This is handled individually for each code.  */
      else
	*total += IA16_COST (add[O_MEMREG]) - IA16_COST (add[O_REGREG]);
#endif
      *total *= H_MOD (mode);
      return (true);

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
    case CONST_INT:
    case CONST_VECTOR:
      *total = ia16_constant_cost (x, mode, outer_code);
      /* Take counter measures against prepare_cmp_insn().  */
      if (outer_code == COMPARE && *total > COSTS_N_INSNS (1))
	*total = COSTS_N_INSNS (1);
      /* This is never called recursively from the SET case.  */
      if (outer_code == SET)
	*total += IA16_COST (imm_load[M_HI]);
      return true;

    case CONST_DOUBLE:
      *total = ia16_costs->byte_fetch * COSTS_N_INSNS (4);
      return true;

    /* This is usually "xor reg,reg" or "mov $0, dest".  */
    case ZERO_EXTEND:
      if (outer_code == IOR || outer_code == XOR)
	*total = 0;
      else
	*total = IA16_COST (add[I_RTX (XEXP (x, 0))]);

      /* Avoid the extra high cost of a subreg.  */
      if (GET_CODE (XEXP (x, 0)) == SUBREG)
	{
	  *total += rtx_cost (SUBREG_REG (XEXP (x, 0)), mode, code, 0, speed);
	  return true;
	}
      return false;

    case SIGN_EXTEND:
      *total = IA16_COST (sign_extend[M_MOD (GET_MODE (XEXP (x, 0)))]);
      return false;

    /* TODO: Constant shift/rotate when !TARGET_SHIFT_IMM.  */
    case ASHIFTRT:
      if (outer_code != COMPARE && CONST_INT_P (XEXP (x, 1)))
	{
	  /* This is implemented using sign extension.  */
	  if (mode == QImode && INTVAL (XEXP (x, 1)) >= 7)
	    if (outer_code == SIGN_EXTEND)
	      *total = 0;
	    else
	      *total = IA16_COST (sign_extend[M_QI]);
	  else if (mode == HImode && INTVAL (XEXP (x, 1)) >= 15)
	    *total = IA16_COST (sign_extend[M_HI]);
          *total += rtx_cost (XEXP (x, 0), mode, code, 0, speed);
          return true;
	}
      /* fall through */
    case ASHIFT:
      /* Sometimes we get the ASHIFT and the constant for free.  */
      if (code == ASHIFT && mode == HImode
	  && (outer_code == PLUS || outer_code == MINUS
	     || outer_code == IOR || outer_code == XOR)
          && CONST_INT_P (XEXP (x, 1))
          && INTVAL (XEXP (x, 1)) == 8)
	{
	  *total = rtx_cost (XEXP (x, 0), mode, code, 0, speed);
	  return true;
	}
    case LSHIFTRT:
      /* Similiarly with LSHIFTRT.  */
      if (code == LSHIFTRT && mode == HImode
	  && (outer_code == IOR || outer_code == XOR)
	  && CONST_INT_P (XEXP (x, 1))
	  && INTVAL (XEXP (x, 1)) == 8)
	{
	  *total = rtx_cost (XEXP (x, 0), mode, code, 0, speed);
	  return true;
        }
    case ROTATE:
    case ROTATERT:
      if (CONST_INT_P (XEXP (x, 1)))
	if (mode == HImode && outer_code != COMPARE
	 && INTVAL (XEXP (x, 1)) == 8)
	  {
	    /* This is implemented using movb or xchgb.  */
	    *total = IA16_COST (add[O_REGREG])
	      + rtx_cost (XEXP (x, 0), mode, code, 0, speed);
	    return true;
	  }
	else if (INTVAL (XEXP (x, 1)) == 1)
	  {
	    *total = IA16_COST (shift_1bit[I_REG])
	      + rtx_cost (XEXP (x, 0), mode, code, 0, speed);
	  }
	else
	  *total = MAX (ia16_costs->shift_start[I_REG]
		        + ia16_costs->shift_bit * INTVAL (XEXP (x, 1)),
			ia16_size_costs.shift_start[I_REG]
			* ia16_costs->byte_fetch);
      else
	*total = MAX (ia16_costs->shift_start[I_REG]
		      + ia16_costs->shift_bit
			* GET_MODE_BITSIZE (GET_MODE (XEXP (x, 1))) / 2,
		      ia16_size_costs.shift_start[I_REG]
			* ia16_costs->byte_fetch);

      /* (subreg:QI (ashift:HI (zero_extend:HI (mem:QI ...))) 0) */
      if (outer_code == SUBREG
	  && (ZERO_EXTEND == GET_CODE (XEXP (x, 0))
	      || SIGN_EXTEND == GET_CODE (XEXP (x,0))))
	{
	  *total += rtx_cost (XEXP (XEXP (x, 0), 0), mode, code, 0, speed);
	  return true;
	}
      else
	return false;

    case MULT:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fmul);
	  return false;
	}
      else
	{
	  rtx op0 = XEXP (x, 0);
	  rtx op1 = XEXP (x, 1);
	  int nbits;

	  /* (mult:HI (...) (const_int 257) is cheap.  */
	  if (CONST_INT_P (op1) && INTVAL (op1) == 257 && mode == HImode)
	    {
	      *total = IA16_COST (add[O_REGREG])
		+ rtx_cost (op0, mode, code, 0, speed);
	      return true;
            }
	  if (CONST_INT_P (op1) && ia16_costs->mult_bit != 0)
	    {
	      unsigned HOST_WIDE_INT value = INTVAL (XEXP (x, 1));
	      for (nbits = 0; value != 0; value &= value - 1)
	        nbits++;
	    }
	  else
	    /* This is arbitrary.  */
	    nbits = 7;

	  /* Compute costs correctly for widening multiplication.  */
	  if ((GET_CODE (op0) == SIGN_EXTEND || GET_CODE (op1) == ZERO_EXTEND)
	      && GET_MODE_SIZE (GET_MODE (XEXP (op0, 0))) * 2
	         == GET_MODE_SIZE (mode))
	    {
	      int is_mulwiden = 0;
	      enum machine_mode inner_mode = GET_MODE (op0);

	      if (GET_CODE (op0) == GET_CODE (op1))
		is_mulwiden = 1, op1 = XEXP (op1, 0);
	      else if (CONST_INT_P (op1))
		{
		  if (GET_CODE (op0) == SIGN_EXTEND)
		    is_mulwiden = trunc_int_for_mode (INTVAL (op1), inner_mode)
			          == INTVAL (op1);
		  else
		    is_mulwiden = !(INTVAL (op1) & ~GET_MODE_MASK (inner_mode));
	        }

	      if (is_mulwiden)
	        op0 = XEXP (op0, 0), mode = GET_MODE (op0);
	    }

	  /* FIXME: Check for signed vs. unsigned.  */
  	  *total = MAX (ia16_costs->s_mult_init[M_MOD (mode)][I_RTX (op1)]
			+ nbits * ia16_costs->mult_bit,
			ia16_size_costs.s_mult_init[M_MOD (mode)][I_RTX (op1)]
			* ia16_costs->byte_fetch)
	    + rtx_cost (op0, mode, outer_code, 0, speed)
	    + rtx_cost (op1, mode, outer_code, 1, speed);

          return true;
	}

    /* FIXME: Docs are unclear about UDIV/UMOD on floating point modes.  */
    case UDIV:
    case UMOD:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fdiv);
      else
	*total = IA16_COST (u_divide[M_MOD (mode)][I_RTX (XEXP (x, 0))]);
      return false;

    case DIV:
    case MOD:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fdiv);
      else
	*total = IA16_COST (s_divide[M_MOD (mode)][I_RTX (XEXP (x, 0))]);
      return false;

    /* FIXME: This might be too pessimistic when rtx_equal_p (op0, op1).  */
    case PLUS:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fadd);
	  return false;
	}
      /* PLUS with -1 or 1 is cheaper.  */
      if (XEXP (x, 1) == const1_rtx || XEXP (x, 1) == constm1_rtx)
	{
	  *total = IA16_COST (inc_dec[I_RTX (x)]);
	  return (false);
	}
      /* "shl $1, dest" is preferred over "add dest, dest".  */
      if (GET_MODE_SIZE (mode) <= 2
	  && rtx_equal_p (XEXP (x, 0), XEXP (x, 1)))
	{
	  *total = 1 + IA16_COST (add[I_RTX (x)])
		   * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
	  return (false);
	}
      /* Compute cost of "leaw" instruction.  */
      if (mode == HImode)
	{
	  if (GET_CODE (XEXP (x, 0)) == PLUS
	    && CONSTANT_P (XEXP (x,1)))
	    {
	      rtx op0 = XEXP (XEXP (x, 0), 0);
	      rtx op1 = XEXP (XEXP (x, 0), 1);
	      rtx cst = XEXP (x, 1);

	      *total = IA16_COST (lea)
		+ IA16_COST (ea_calc (op0, op1, cst, NULL_RTX))
		+ rtx_cost (op0, mode, outer_code, 0, speed)
	        + rtx_cost (op1, mode, outer_code, 1, speed);
	      return true;
	    }
	}
      /* Detect *subm3_carru.  */
      if (outer_code == MINUS
	  && carry_flag_operator (XEXP (x, 0), mode))
	{
	  *total = rtx_cost (XEXP (x, 1), mode, outer_code, 1, speed);
	  return false;
	}
      /* FALLTHRU */

    case MINUS:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fadd);
	  return false;
	}
      /* Detect *addm3_carry.  */
      if (outer_code == PLUS
	  && carry_flag_operator (XEXP (x, 1), mode))
	{
	  *total = rtx_cost (XEXP (x, 0), mode, outer_code, 0, speed);
	  return false;
	}
      /* FALLTHRU */

    case AND:
      /* Sometimes we get both the AND and the constant for free.  */
      if (code == AND && mode == HImode
	  && (outer_code == PLUS || outer_code == MINUS
	     || outer_code == IOR || outer_code == XOR)
          && CONST_INT_P (XEXP (x, 1))
          && INTVAL (XEXP (x, 1)) == -256)
	{
	  *total = rtx_cost (XEXP (x, 0), mode, code, 0, speed);
	  return true;
        }
    case IOR:
    case XOR:
      if (CONSTANT_P (XEXP (x, 1)))
	*total = IA16_COST (add_imm[I_RTX (XEXP (x, 0))])
	       * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      else
	*total = IA16_COST (add[I_RTX (XEXP (x, 0))])
	       * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      return false;

    case NEG:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fchs);
	  return false;
	}
      if (carry_not_flag_operator (XEXP (x, 0), mode))
	{
	  *total = 0;
	  return false;
        }
      /* FALLTHRU */

    case NOT:
      *total = IA16_COST (add[I_RTX (XEXP (x, 0))])
	     * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      return false;

    case COMPARE:
      if (GET_CODE (XEXP (x, 0)) == ZERO_EXTRACT
	  && XEXP (XEXP (x, 0), 1) == const1_rtx
	  && CONST_INT_P (XEXP (XEXP (x, 0), 2))
	  && XEXP (x, 1) == const0_rtx)
	{
	  /* This kind of construct is implemented using test[bw].
	     Treat it as if we had an AND.  */
	  *total = (IA16_COST (add_imm[I_RTX (x)])
		    + rtx_cost (XEXP (XEXP (x, 0), 0), mode, outer_code, 0,
				speed)
		    + rtx_cost (const1_rtx, mode, outer_code, 1, speed));
	  return true;
	}
      /* COMPARE is free in e.g. (compare (plus ...) (const_int 0)).  */
      if ((GET_RTX_CLASS (GET_CODE (XEXP (x, 0))) == RTX_COMM_ARITH
	   || GET_RTX_CLASS (GET_CODE (XEXP (x, 0))) == RTX_BIN_ARITH)
	  && XEXP (x, 1) == const0_rtx)
	{
	  *total = rtx_cost (XEXP (x, 0), mode, outer_code, 0, speed);
	  return true;
	}
      /* COMPARE is free in overflow checks.  */
      if (GET_CODE (XEXP (x, 0)) == MINUS
	  && rtx_equal_p (XEXP (x, 1), XEXP (XEXP (x, 0), 0)))
	{
	  *total = rtx_cost (XEXP (x, 0), mode, outer_code, 0, speed);
	  return true;
	}
      if (CONSTANT_P (XEXP (x, 1)))
	{
	  *total = IA16_COST (cmp_imm[I_RTX (XEXP (x, 0))]);
	  if (XEXP (x, 1) == const0_rtx
	      && (REG_P (XEXP (x, 0)) || GET_CODE (XEXP (x, 0)) == SUBREG))
	    return true;
	}
      else
	*total = IA16_COST (cmp[I_RTX (XEXP (x, 0))]);
      return false;

    case ZERO_EXTRACT:
      if (outer_code == COMPARE)
	{
	  /* See above under COMPARE.  */
	  *total = (IA16_COST (add[I_RTX (x)])
		    + rtx_cost (XEXP (x, 0), mode, outer_code, 0, speed)
		    + rtx_cost (const1_rtx, mode, outer_code, 0, speed));
	  return true;
	}
      if (REG_P (x))
	*total = IA16_COST (move);
      else
	*total = IA16_COST (int_load[M_QI]);
      return false;

    case FLOAT_EXTEND:
	*total = 0;
      return false;

    case ABS:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fabs);
      else
	/* This is expanded to cwtd, xorw and subw.  */
	*total = IA16_COST (sign_extend[M_MOD (mode)])
	       + IA16_COST (add[I_REG]) * 2;
      return false;

    case SQRT:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fsqrt);
      return false;

    case TRUNCATE:
      *total = 0;
      return true;

    case CALL:
      if (CONSTANT_P (XEXP (x, 0)))
	*total = IA16_COST (call[0]);
      else if (REG_P (XEXP (x, 0)))
	*total = IA16_COST (call[1]);
      else
	*total = IA16_COST (call[2]);
      return false;

    case LTU:
    case GEU:
    case LEU:
    case GTU:
    case GT:
    case LT:
    case LE:
    case GE:
    case NE:
    case EQ:
      if (outer_code != SET && outer_code != NEG)
	*total = 0;
      else if (carry_not_flag_operator (x, mode))
	*total = IA16_COST (add[O_REGREG])
	       + outer_code == SET ? COSTS_N_INSNS (ia16_costs->byte_fetch) : 0
	       + outer_code == NEG ? IA16_COST (add[I_REG]) : 0;
      else if (outer_code != SET)
	*total = 0;
      else if (carry_flag_operator (x, mode))
	*total = IA16_COST (add[O_REGREG]);
      else if (code == LT)
	*total = COSTS_N_INSNS (ia16_costs->byte_fetch)
	       + IA16_COST (sign_extend[M_MOD (HImode)]);
      else if (code == EQ)
	*total = COSTS_N_INSNS (ia16_costs->byte_fetch)
	       + IA16_COST (shift_1bit[I_REG])
	       + IA16_COST (sign_extend[M_MOD (HImode)]);
      return true;

    case UNSPEC:
      /* Instructions like cmc and lahf.  Assume they are cheap.  */
      *total = COSTS_N_INSNS (ia16_costs->byte_fetch);
      return false;

    /* cse_insn() likes to compute costs of EXPR_LIST rtxes.  */
    case EXPR_LIST:

    /* TODO */
    /* Check outer_code here (e.g. SET, MINUS).  */
    case IF_THEN_ELSE:
    case PC:
    case CLZ:
    case CTZ:
    case FFS:
    case PARITY:
    case POPCOUNT:
    case ASM_INPUT:
    case ASM_OPERANDS:
    case CONCAT:
    case STRICT_LOW_PART:
      return false;

    /* See comment above get_last_value (const_rtx) in combine.c.  */
    case CLOBBER:
      return false;

    default:
      fprintf (stderr, "Unknown cost for rtx with code %s and "
               "outer code %s:\n", GET_RTX_NAME (code),
	       GET_RTX_NAME (outer_code));
      debug_rtx (x);
      return false;
    }
}

/* Dividing the Output into Sections */
#undef	TARGET_ASM_SELECT_SECTION
#define	TARGET_ASM_SELECT_SECTION ia16_asm_select_section

/* Convenience function --- fabricate a section name for a given __far
   variable or function declaration, or return NULL if something is wrong.

   The caller should free the section name's memory with free (.) once it is
   done with it.

   To fabricate the section name, I first decide the prefix for the section
   type, then add the name of the variable; and finally I compute a hash
   value of everything and append the hash.

   (Note: apparently CFUN tends to be NULL at this point, even if the
   variable is defined inside a function.)  */
static char *
ia16_fabricate_section_name_for_decl (tree decl, int reloc)
{
  const char *prefix;
  const char *dname;
  char *name1, *name2;
  unsigned short hash;
  bool one_only;

  if (! DECL_P (decl))
    return NULL;

  if (! TARGET_SEG_RELOC_STUFF)
    {
      location_t loc = DECL_SOURCE_LOCATION (decl);
      error_at (loc,
		"cannot create %<__far%> function or static storage variable");
      ia16_error_seg_reloc (loc, NULL);
      return NULL;
    }

  one_only = DECL_COMDAT_GROUP (decl) && ! HAVE_COMDAT_GROUP;

  switch (categorize_decl_for_section (decl, reloc))
    {
    case SECCAT_TEXT:
      prefix = one_only ? ".gnu.linkonce.ft." : ".fartext.";
      break;

    case SECCAT_DATA:
    case SECCAT_DATA_REL:
    case SECCAT_DATA_REL_LOCAL:
    case SECCAT_BSS:
      prefix = one_only ? ".gnu.linkonce.fd." : ".fardata.";
      break;

    case SECCAT_DATA_REL_RO:
    case SECCAT_DATA_REL_RO_LOCAL:
    case SECCAT_RODATA:
    case SECCAT_RODATA_MERGE_STR:
    case SECCAT_RODATA_MERGE_STR_INIT:
    case SECCAT_RODATA_MERGE_CONST:
      prefix = one_only ? ".gnu.linkonce.fr." : ".farrodata.";
      break;

    default:
      return NULL;
    }

  dname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
  dname = targetm.strip_name_encoding (dname);

  name1 = ACONCAT ((prefix, dname, NULL));
  hash = (unsigned short) htab_hash_string (name1);
  hash *=  036627u;
  hash &= 0177777u;
  hash = hash << 4 | hash >> 12;
  hash &=  077777u;

  if (asprintf (&name2, "%s.%05ho", name1, hash) <= 0)
    {
      error ("not enough memory for %<__far%> function or variable "
	     "section name");
      return NULL;
    }

  return name2;
}

static section *
ia16_asm_select_section (tree expr, int reloc, unsigned HOST_WIDE_INT align)
{
  tree type = TREE_TYPE (expr);
  char *sname;

  switch (TYPE_ADDR_SPACE (type))
    {
    default:
      gcc_unreachable ();

    case ADDR_SPACE_FAR:
      sname = ia16_fabricate_section_name_for_decl (expr, reloc);
      if (sname)
	{
	  section *sect = get_named_section (expr, sname, reloc);
	  free (sname);
	  return sect;
	}

      /* fall through */
    case ADDR_SPACE_GENERIC:
      return default_elf_select_section (expr, reloc, align);
    }
}

#undef	TARGET_ASM_UNIQUE_SECTION
#define	TARGET_ASM_UNIQUE_SECTION ia16_asm_unique_section

static void
ia16_asm_unique_section (tree decl, int reloc)
{
  char *sname;
  tree type = TREE_TYPE (decl);

  switch (TYPE_ADDR_SPACE (type))
    {
    default:
      gcc_unreachable ();

    case ADDR_SPACE_FAR:
      sname = ia16_fabricate_section_name_for_decl (decl, reloc);
      if (sname)
	{
	  set_decl_section_name (decl, sname);
	  free (sname);
	  return;
	}

      /* fall through */
    case ADDR_SPACE_GENERIC:
      default_unique_section (decl, reloc);
    }
}

/* Continued: Run-time Target Specification */
#undef  TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE	ia16_option_override
static void
ia16_option_override (void)
{
  ia16_costs = processor_target_table[target_arch].cost;
  ia16_features = processor_target_table[target_arch].features;
  if (target_tune != PROCESSOR_ANY)
    {
      ia16_costs = processor_target_table[target_tune].cost;
      ia16_features = (ia16_features & 15)
	| (processor_target_table[target_tune].features & 16);
    }
  if (optimize_size)
    ia16_costs = &ia16_size_costs;
}

/* The Overall Framework of an Assembler File */
#undef  TARGET_ASM_FILE_START
#undef  TARGET_ASM_FILE_START_APP_OFF
#define TARGET_ASM_FILE_START			ia16_asm_file_start
#define TARGET_ASM_FILE_START_APP_OFF		1

/* GAS doesn't support .file.  */
#undef  TARGET_ASM_FILE_START_FILE_DIRECTIVE
#define TARGET_ASM_FILE_START_FILE_DIRECTIVE	false

/* Output assembler directives to set up the GNU assembler for the CPU type,
 * code model and assembler syntax that we use.  */
static void
ia16_asm_file_start (void)
{
	const char *arch, *code;

	if (TARGET_FSTSW_AX)
		arch = "i286";
	else if (TARGET_PUSH_IMM)
		arch = "i186";
	else
		arch = "i8086";
	code = "16";

	fprintf (asm_out_file,	"\t.arch %s,jumps\n"
				"\t.code%s\n"
				"\t.att_syntax prefix\n", arch, code);
	default_file_start ();
}

#undef	TARGET_ASM_FUNCTION_SECTION
#define	TARGET_ASM_FUNCTION_SECTION	ia16_asm_function_section

static section *
ia16_asm_function_section (tree decl, enum node_frequency freq, bool startup,
			   bool stop)
{
  char *sname;
  const int reloc = 0;

  if (! decl
      || TYPE_ADDR_SPACE (TREE_TYPE (decl)) != ADDR_SPACE_FAR
      || ! ia16_far_section_function_type_p (TREE_TYPE (decl)))
    return default_function_section (decl, freq, startup, stop);

  sname = ia16_fabricate_section_name_for_decl (decl, reloc);
  if (sname)
    {
      section *sect = get_named_section (decl, sname, reloc);
      free (sname);
      return sect;
    }
  return NULL;
}

/* Output of Data */
#undef  TARGET_ASM_BYTE_OP
#undef  TARGET_ASM_ALIGNED_HI_OP
#undef  TARGET_ASM_ALIGNED_SI_OP
#undef  TARGET_ASM_ALIGNED_DI_OP
#undef  TARGET_ASM_ALIGNED_TI_OP
#undef  TARGET_ASM_UNALIGNED_HI_OP
#undef  TARGET_ASM_UNALIGNED_SI_OP
#undef  TARGET_ASM_UNALIGNED_DI_OP
#undef  TARGET_ASM_UNALIGNED_TI_OP
#define TARGET_ASM_BYTE_OP		"\t.byte\t"
#define TARGET_ASM_ALIGNED_HI_OP	"\t.hword\t"
#define TARGET_ASM_ALIGNED_SI_OP	"\t.long\t"
#define TARGET_ASM_ALIGNED_DI_OP	"\t.quad\t"
#define TARGET_ASM_ALIGNED_TI_OP	"\t.octa\t"
#define TARGET_ASM_UNALIGNED_HI_OP	"\t.hword\t"
#define TARGET_ASM_UNALIGNED_SI_OP	"\t.long\t"
#define TARGET_ASM_UNALIGNED_DI_OP	"\t.quad\t"
#define TARGET_ASM_UNALIGNED_TI_OP	"\t.octa\t"

#undef	TARGET_ASM_INTEGER
#define	TARGET_ASM_INTEGER	ia16_asm_integer

static rtx
ia16_find_base_symbol_ref (rtx x)
{
  rtx b0;

  switch (GET_CODE (x))
    {
    case SYMBOL_REF:
      return x;

    case PLUS:
      b0 = ia16_find_base_symbol_ref (XEXP (x, 0));
      if (b0)
	return b0;
      return ia16_find_base_symbol_ref (XEXP (x, 1));

    case MINUS:
      return ia16_find_base_symbol_ref (XEXP (x, 0));

    default:
      return NULL_RTX;
    }
}

static bool
ia16_asm_integer (rtx x, unsigned size, int aligned_p)
{
  rtx base;

  if (default_assemble_integer (x, size, aligned_p))
    return true;

  while (GET_CODE (x) == CONST)
    x = XEXP (x, 0);

  if (GET_CODE (x) != UNSPEC
      || XINT (x, 1) != UNSPEC_STATIC_FAR_PTR
      || GET_MODE (x) != SImode)
    return false;

  x = XVECEXP (x, 0, 0);
  base = ia16_find_base_symbol_ref (x);

  if (! base)
    return false;

  fputs (TARGET_ASM_ALIGNED_HI_OP, asm_out_file);
  output_addr_const (asm_out_file, x);

  /* GNU as does not yet accept the syntax
	.hword	foo@SEGMENT16
     so we have to say
	.reloc	., R_386_SEGMENT16, foo
	.hword	0
     instead.  */
  fputs ("\n"
	 "\t.reloc\t., R_386_SEGMENT16, ", asm_out_file);
  output_addr_const (asm_out_file, base);
  fputs ("\n" TARGET_ASM_ALIGNED_HI_OP "0\n", asm_out_file);
  return true;
}

static const char *reg_QInames[FIRST_NOQI_REG] = {
	"cl", "ch", "al", "ah", "dl", "dh", "bl", "bh"
};

static const char *reg_HInames[LAST_HARD_REG + 1] = {
	"cx", 0, "ax", 0, "dx", 0, "bx", 0, "si", "di", "bp", "es", "ds", "sp",
	0, "ss", "cs"
};

/* E is known not to be null when this is called.  These non-standard codes are
   supported:
   'L': Print the name of the lower 8-bits of E.
   'H': Print the name of the upper 8-bits of E.
   'X': Print the name of E as a 16-bit operand.
   'W': Print the value of the constant E unsigned-divided by 2 (for loading
	%cx before a `rep; movsw').
   'R': Don't print any register prefix before E.
   'O': Print the low 16 bits of the 32-bit constant E (for immediate
        `lcall' and `ljmp').
   'S': Print the high 16 bits of the 32-bit constant E.
*/
void
ia16_print_operand (FILE *file, rtx e, int code)
{
  enum machine_mode mode;
  unsigned int regno;
  rtx x;

  /* simplify_gen_subreg() is used to allow the 'X', 'H', 'L', and 'W' output
   * modifiers to be used for REG, MEM and constant expressions.  The cases
   * which simplify_subreg() refuses to handle are handled by alter_subreg().
   */
  if (code == 'X')
    x = simplify_gen_subreg (HImode, e, QImode, 0);
  else if (code == 'L')
    if (GET_MODE (e) == VOIDmode)
      x = simplify_gen_subreg (QImode, e, HImode, 0);
    else
      x = simplify_gen_subreg (QImode, e, GET_MODE (e), 0);
  else if (code == 'H')
    x = simplify_gen_subreg (QImode, e, HImode, 1);
  else if (code == 'W')
    {
      gcc_assert (CONST_INT_P (e));
      x = GEN_INT ((INTVAL (e) / 2) & 0x7fff);
    }
  else if (code == 'O')
    {
      gcc_assert (CONST_INT_P (e));
      x = GEN_INT (INTVAL (e) & 0xffff);
    }
  else if (code == 'S')
    {
      gcc_assert (CONST_INT_P (e));
      x = GEN_INT ((INTVAL (e) >> 16) & 0xffff);
    }
  else
    x = e;
  if (GET_CODE (x) == SUBREG)
    x = alter_subreg (&x, true);
  mode = GET_MODE (x);

  switch (GET_CODE (x))
    {
      case REG:
      regno = REGNO (x);
      if (code != 'R')
	fputs (REGISTER_PREFIX, file);
      if (GET_MODE_SIZE (mode) >= 2
       && (regno >= SI_REG || (regno & 1) == 0))
	fputs (reg_HInames[regno], file);
      else if (GET_MODE_SIZE (mode) == 1 && regno < SI_REG)
	fputs (reg_QInames[regno], file);
      else
	output_operand_lossage ("Invalid register %s (nr. %u) in %smode.",
	                        reg_names [REGNO (x)], REGNO (x),
	                        GET_MODE_NAME (GET_MODE (x)));
      break;

      case CONST_VECTOR:
      fprintf (file, "%s" HOST_WIDE_INT_PRINT_DEC, IMMEDIATE_PREFIX,
	       (UINTVAL (XVECEXP (x, 0, 0)) & 0xff)
	       + 256 * INTVAL (XVECEXP (x, 0, 1)));
      break;

      /* TODO: handle floating point constants here.  */

      case CONST_INT:
      case CONST:
      case SYMBOL_REF:
      case LABEL_REF:
      fputs (IMMEDIATE_PREFIX, file);
      /* fall through */

      case CODE_LABEL:
      output_addr_const (file, x);
      break;

      case MEM:
      ia16_print_operand_address (file, XEXP (x, 0));
      break;

      default:
      debug_rtx (e);
      output_operand_lossage ("Invalid or unsupported operand %s (code"
			      " %c).", rtx_name [GET_CODE (e)], code);
	break;
  }
}

#define INDEX_REG_P(x) \
	ia16_regno_in_class_p (REGNO (x), INDEX_REGS)
#define BASE_REG_REG_P(x) \
	ia16_regno_in_class_p (REGNO (x), BASE_W_INDEX_REGS)

/* Strictly check an address X and optionally split into its components.
 * If there is only one register, it will be the base register.
 * This is a helper function for ia16_print_operand_address ().
 */
static bool
ia16_parse_address_strict (rtx x, rtx *p_rb, rtx *p_ri, rtx *p_c, rtx *p_rs)
{
	rtx tmp;
	rtx rb, ri, c, rs;
	enum machine_mode mode ATTRIBUTE_UNUSED;

	if (!ia16_parse_address (x, &rb, &ri, &c, &rs))
		return (0 == 1);
	mode = GET_MODE (x);

	/* Swap the registers if necessary.  */
	if (rb && ri && INDEX_REG_P (rb) && BASE_REG_REG_P (ri))
	  {
		tmp = rb; rb = ri; ri = tmp;
	  }
	/* Check register classes for base + index.  */
	if (rb && ri
	    && (!INDEX_REG_P (ri)
		|| !BASE_REG_REG_P (rb)))
		return (0 == 1);

	/* Check register class for base.  */
	if (rb && !ri && !REGNO_MODE_OK_FOR_BASE_P (REGNO (rb), mode))
		return (0 == 1);

	/* Check register class for segment. */
	if (rs && !REGNO_OK_FOR_SEGMENT_P (REGNO (rs)))
		return (0 == 1);

	if (p_rb)
		*p_rb = rb;
	if (p_ri)
		*p_ri = ri;
	if (p_c)
		*p_c = c;
	if (p_rs)
		*p_rs = rs;

	return (0 == 0);
}

static bool
ia16_to_print_seg_override_p (unsigned seg_regno, rtx rb)
{
  if (rb && REG_P (rb) && REGNO (rb) == BP_REG)
    {
      if (seg_regno == SS_REG)
	return false;
    }
  else
    {
      if (seg_regno == DS_REG)
	return false;
    }
  return true;
}

/* Possible asm operands and their address expressions:
 * c:		(const_int c)
 * (rb):	(reg rb)
 * c(rb):	(plus (reg rb) (const_int c))
 * (rb, ri):	(plus (reg rb) (reg ri))
 * c(rb, ri):	(plus (plus (reg rb) (reg ri)) (const_int c))
 * rs:0:	(unspec rs ...)
 * rs:(rb)	(plus (unspec rs ...) (reg rb))
 *		etc.
 */
void
ia16_print_operand_address (FILE *file, rtx e)
{
  rtx rb, ri, c, rs;

  if (!ia16_parse_address_strict (e, &rb, &ri, &c, &rs))
    {
      debug_rtx (e);
      output_operand_lossage ("Invalid IA16 address expression.");
      return;
    }

  /* Map our RTL semantics to the x86 instruction set's segment override
     weirdness.

     On the RTL side:
       * A null segment override (RS) means we are accessing an operand in
	 the generic address space.  We currently assume that %ss points to
	 this space.  %ds _might_ point to this space --- if the function
	 assumes that %ds == .data on startup, and if we are compiling does
	 not use %ds for anything (liveness information for %ds is available
	 to us at this stage).
       * If RS is not null, then we are accessing an operand at an offset
	 from the specified segment.

     On the machine code side, no segment override is needed if
       * our operand is an offset from %ss, and the offset involves the
	 register %bp; or
       * our operand is an offset from %ds, and %bp is not involved.  */
  if (rs)
    {
      if (ia16_to_print_seg_override_p (REGNO (rs), rb))
	fprintf (file, "%s%s:", REGISTER_PREFIX, reg_HInames[REGNO (rs)]);
    }
  else if (TARGET_ALLOCABLE_DS_REG ? df_regs_ever_live_p (DS_REG)
				   : ! ia16_in_ds_data_function_p ())
    {
      if (ia16_to_print_seg_override_p (SS_REG, rb))
	fprintf (file, "%s%s:", REGISTER_PREFIX, reg_HInames[SS_REG]);
    }

  if (c)
    output_addr_const (file, c);
  if (rb)
    fprintf (file, "(%s%s", REGISTER_PREFIX, reg_HInames[REGNO (rb)]);
  if (ri)
    fprintf (file, ",%s%s", REGISTER_PREFIX, reg_HInames[REGNO (ri)]);
  if (rb)
    putc (')', file);
}

/* Helper function for gen_prologue().  Generates RTL to push REGNO, which
 * must be a 16-bit register, onto the stack.  */
rtx
ia16_push_reg (unsigned int regno)
{
  if (TARGET_PROTECTED_MODE && REGNO_OK_FOR_SEGMENT_P (regno))
    return gen__pushphi1_prologue (gen_rtx_REG (PHImode, regno));

  return gen__pushhi1_prologue (gen_rtx_REG (HImode, regno));
}

/* Helper function for gen_epilogue().  Generates RTL to pop REGNO, which
 * must be a 16-bit register, off the stack.  */
rtx
ia16_pop_reg (unsigned int regno)
{
  machine_mode mode = HImode;
  if (TARGET_PROTECTED_MODE && REGNO_OK_FOR_SEGMENT_P (regno))
    mode = PHImode;
  return gen_rtx_SET (gen_rtx_REG (mode, regno),
		      gen_frame_mem (mode,
				     gen_rtx_POST_INC (Pmode,
						       stack_pointer_rtx)));
}

/* Trampolines for Nested Functions */

#undef TARGET_TRAMPOLINE_INIT
#define TARGET_TRAMPOLINE_INIT ia16_trampoline_init

/* tr = trampoline addr, fn = function addr, sc = static chain */
/* FIXME: TARGET_CODE32 is not supported - add data16 and addr16 prefixes.  */
void
ia16_trampoline_init (rtx tr, tree fn, rtx sc)
{
  rtx fn_disp;
  rtx mem;

  /* Opcode register encoding   sBDSbdac for sp bp di si bx dx ax cx */
  unsigned long int regtable = 045763201;

  /* Base opcode for movw $imm16, reg16.  */
  unsigned char mov_opcode = 0xb8;
  unsigned char regno = STATIC_CHAIN_REGNUM;

  if (! TARGET_CMODEL_IS_TINY)
    {
      sorry ("Trampolines are disabled under non-tiny memory models.");
      abort ();
    }

  if (ia16_regno_in_class_p (regno, QI_REGS))
    regno /= 2;
  else
    regno -= 4;
  mov_opcode |= (regtable >> (regno * 3)) & 7;

  /* Write the movw opcode.  */
  mem = adjust_address (tr, QImode, 0);
  emit_move_insn (mem, gen_int_mode (mov_opcode, QImode));

  /* Write the static chain value.  */
  mem = adjust_address (tr, HImode, 1);
  emit_move_insn (mem, sc);

  /* Write the absolute jmp opcode.  */
  mem = adjust_address (tr, QImode, 3);
  emit_move_insn (mem, gen_int_mode (0xe9, QImode));

  /* Calculate the difference between the function address and %pc, which
     points to (tr+4)+2 when the jmp instruction executes.  */
  mem = adjust_address (tr, Pmode, 4 + 2);
  fn_disp = expand_binop (Pmode, sub_optab, XEXP (DECL_RTL (fn), 0),
			  plus_constant (Pmode, XEXP (tr, 0), 4 + 2), NULL_RTX,
			  1, OPTAB_DIRECT);

  /* Write the jmp offset.  */
  mem = adjust_address (tr, HImode, 4);
  emit_move_insn (mem, fn_disp);

  /* Done.  That wasn't a lot of fun.  */
}

/* Subroutine of ia16_parse_address.  */
bool
ia16_parse_address_internal (rtx e, rtx *p_r1, rtx *p_r2, rtx *p_c, rtx *p_r9)
{
  rtx r1 = NULL, r2 = NULL, c = NULL, r9 = NULL;

  if (REG_P (e))
    r1 = e;
  else if (GET_CODE (e) == UNSPEC && XINT (e, 1) == UNSPEC_SEG_OVERRIDE)
    r9 = XVECEXP (e, 0, 0);
  else if (GET_CODE (e) == PLUS)
    {
      rtx x, y, x_r1, x_r2, x_c, x_r9, y_r1, y_r2, y_c, y_r9;

      x = XEXP (e, 0);
      y = XEXP (e, 1);

      if (!ia16_parse_address_internal (x, &x_r1, &x_r2, &x_c, &x_r9) ||
	  !ia16_parse_address_internal (y, &y_r1, &y_r2, &y_c, &y_r9))
	return false;

      if ((x_c && y_c) || (x_r9 && y_r9))
	return false;

      if (x_r1 && y_r1)
	{
	  if (x_r2 || y_r2)
	    return false;

	  /* replace_oldest_value_addr(...) in gcc/regcprop.c may get
	   * confused if an address expression contains both index and base
	   * registers, and the registers do not come immediately under the
	   * same (plus ...), e.g.
	   *
	   *	                (plus:HI)
	   *	                  /  \
	   *	          (plus:HI)  (reg:HI 10 bp)
	   *	            /  \
	   *	(reg:HI 9 di)  (const_int -1)
	   *
	   * In such a case, gcc/regcprop.c may decide to replace %di with
	   * %bx (say), while not replacing %bp, leading to a bogus address
	   * `-1(%bx,%bp)'.
	   *
	   * One way to work around this is to flag address like the above as
	   * being invalid.
	   */
	  if (x_r1 != x || y_r1 != y)
	    return false;
	}

      if (x_r1)
	{
	  r1 = x_r1;
	  r2 = x_r2 ? x_r2 : y_r1;
	}
      else
	{
	  r1 = y_r1;
	  r2 = y_r2;
	}

      c = x_c ? x_c : y_c;
      r9 = x_r9 ? x_r9 : y_r9;
    }
  else
    c = e;

  /* Deal with the most obvious brokenness.  */
  if ((r1 && !REG_P (r1)) || (r2 && !REG_P (r2)) || (r9 && !REG_P (r9)) ||
      (c && !CONSTANT_P (c)))
    return false;

  if (p_r1)
    *p_r1 = r1;
  if (p_r2)
    *p_r2 = r2;
  if (p_c)
    *p_c = c;
  if (p_r9)
    *p_r9 = r9;

  return true;
}

/* Addressing Modes.  */
/* Check an address E and optionally split it into its components.
*/
bool
ia16_parse_address (rtx e, rtx *p_r1, rtx *p_r2, rtx *p_c, rtx *p_r9)
{
  rtx r1, c;
  if (ia16_parse_address_internal (e, &r1, p_r2, &c, p_r9))
    {
      if (p_r1)
	*p_r1 = r1;
      if (p_c)
	{
	  if (c || r1)
	    *p_c = c;
	  else
	    *p_c = const0_rtx;
	}
      return true;
    }
  if (p_r1)
    *p_r1 = NULL;
  if (p_r2)
    *p_r2 = NULL;
  if (p_c)
    *p_c = NULL;
  if (p_r9)
    *p_r9 = NULL;
  return false;
}

/* Miscellaneous Parameters */

/* Only Intel i80186 and i80286 mask the shift/rotate counts.  */
/* FIXME Documentation bug: unsigned HOST_WIDE_INT instead of int.  */
#undef  TARGET_SHIFT_TRUNCATION_MASK
#define TARGET_SHIFT_TRUNCATION_MASK	ia16_shift_truncation_mask

static unsigned HOST_WIDE_INT
ia16_shift_truncation_mask (enum machine_mode mode)
{
  return (TARGET_SHIFT_MASKED && (mode == HImode || mode == QImode) ? 31 : 0);
}

/* If we are in an __attribute__ ((no_assume_ds_data)) function, and %ds is
   fixed, check whether it calls any __attribute__ ((assume_ds_data)).  Flag
   warnings if it does.  */
static void
ia16_verify_calls_from_no_assume_ds (void)
{
  rtx_insn *insn;

  if (TARGET_ALLOCABLE_DS_REG
      || ia16_in_ds_data_function_p ())
    return;

  for (insn = get_insns(); insn; insn = NEXT_INSN (insn))
    {
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);
	  if (ia16_ds_data_function_rtx_p (callee))
	      warning_at (LOCATION_LOCUS (INSN_LOCATION (insn)),
			  OPT_Wmaybe_uninitialized,
			  "%%ds is fixed, not resetting %%ds for call to "
			  "assume_ds_ss function");
	}
    }
}

/* If we are in an __attribute__ ((far_section)) function, check whether it
   calls any near functions.  Flag warnings if it does.  */
static void
ia16_verify_calls_from_far_section (void)
{
  rtx_insn *insn;

  if (! ia16_in_far_section_function_p ())
    return;

  for (insn = get_insns(); insn; insn = NEXT_INSN (insn))
    {
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);

	  tree fndecl = ia16_get_function_decl_for_addr (callee);
	  tree fntype = NULL_TREE;
	  if (fndecl)
	    {
	      fntype = TREE_TYPE (fndecl);
	      while (POINTER_TYPE_P (fntype))
		fntype = TREE_TYPE (fntype);
	    }

	  if (! fntype || TYPE_ADDR_SPACE (fntype) != ADDR_SPACE_FAR)
	      warning_at (LOCATION_LOCUS (INSN_LOCATION (insn)),
			  OPT_Waddress,
			  "calling near function from outside near text "
			  "segment");
	}
    }
}

/* Return true iff INSN is a (set (reg:SEG DS_REG) (reg:SEG SS_REG)).  */
static bool
ia16_insn_is_reset_ds_p (rtx_insn *insn)
{
  rtx pat, op;

  if (! NONJUMP_INSN_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != SET)
    return false;

  op = SET_DEST (pat);
  if (! REG_P (op) || REGNO (op) != DS_REG || GET_MODE (op) != SEGmode)
    return false;

  op = SET_SRC (pat);
  if (! REG_P (op) || REGNO (op) != SS_REG || GET_MODE (op) != SEGmode)
    return false;

  return true;
}

/* Return true iff INSN is a (use (reg:SEG DS_REG)).  */
static bool
ia16_insn_is_use_ds_p (rtx_insn *insn)
{
  rtx pat, op;

  if (! NONJUMP_INSN_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != USE)
    return false;

  op = XEXP (pat, 0);
  if (! REG_P (op) || REGNO (op) != DS_REG || GET_MODE (op) != SEGmode)
    return false;

  return true;
}

/* Elide unneeded instances of (set (reg:SEG DS_REG) (reg:SEG SS_REG)) in
   the current function.  Also elide any unneeded `%ss:' segment overrides
   from simple moves to/from memory (`ldsw', `cmpw', etc. are not yet
   handled).  */
static void
ia16_elide_unneeded_ss_stuff (void)
{
  int max_insn_uid = get_max_uid ();
  bool *insn_pushed_p = XCNEWVEC (bool, max_insn_uid);
  bool *keep_insn_p = XCNEWVEC (bool, max_insn_uid);
  rtx_insn *insn, **stack = XCNEWVEC (rtx_insn *, max_insn_uid);
  int sp = 0;
  rtx ds = gen_rtx_REG (SEGmode, DS_REG),
      override = ia16_seg_override_term (ds);
  bool default_ds_abi_p = ia16_in_ds_data_function_p ();
  bool keep_any_resets_p = false;
  edge e;
  edge_iterator ei;

  /* Starting from insns which set (or clobber) %ds to not-.data, "flood"
     insns along the control flow until we hit
	(set (reg:SEG DS_REG) (reg:SEG SS_REG)).
     For now we assume %ss == .data always.

     If the current function does not assume %ds == .data on entry, also
     flood from the function entry point.

     Mark all the insns along each path --- excluding the first set to
     non-.data, but including the final %ds <- .data --- as insns we want to
     "keep" as is.  */
  if (! default_ds_abi_p)
    {
      FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR_FOR_FN (cfun)->succs)
	{
	  insn = BB_HEAD (e->dest);
	  if (! insn_pushed_p[INSN_UID (insn)])
	    {
	      stack[sp++] = insn;
	      insn_pushed_p[INSN_UID (insn)] = true;
	    }
	}
    }

  for (insn = get_insns(); insn; insn = NEXT_INSN (insn))
    {
      /* GCC's machine-independent code thinks function calls always clobber
	 %ds, but we know that this is not really true.  */
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);
	  if (ia16_ds_data_function_rtx_p (callee))
	    continue;
	}

      if (NOTE_P (insn)
	  || ! reg_set_p (ds, insn)
	  || ia16_insn_is_reset_ds_p (insn))
	continue;

      if (! insn_pushed_p[INSN_UID (insn)])
	{
	  stack[sp++] = insn;
	  insn_pushed_p[INSN_UID (insn)] = true;
	}
    }

  while (sp != 0)
    {
      rtx_insn *insn;
      basic_block bb;

      gcc_assert (sp > 0 && sp <= max_insn_uid);
      insn = stack[--sp];

      if (ia16_insn_is_reset_ds_p (insn))
	{
	  keep_any_resets_p = true;
	  continue;
	}

      bb = BLOCK_FOR_INSN (insn);
      if (insn != BB_END (bb))
	{
	  insn = NEXT_INSN (insn);
	  keep_insn_p[INSN_UID (insn)] = true;
	  if (! insn_pushed_p[INSN_UID (insn)])
	    {
	      stack[sp++] = insn;
	      insn_pushed_p[INSN_UID (insn)] = true;
	    }
	}
      else
	{
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    {
	      if (e->dest == EXIT_BLOCK_PTR_FOR_FN (cfun))
		continue;

	      insn = BB_HEAD (e->dest);
	      keep_insn_p[INSN_UID (insn)] = true;
	      if (! insn_pushed_p[INSN_UID (insn)])
		{
		  stack[sp++] = insn;
		  insn_pushed_p[INSN_UID (insn)] = true;
		}
	    }
	}
    }

  /* Rescan the whole insn list for
	(set (reg:SEG DS_REG) (reg:SEG SS_REG))
     and simple moves to/from the generic address space, and rewrite those
     which we have not marked as "keep".  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      rtx pat, dest, src;

      if (keep_insn_p[INSN_UID (insn)])
	continue;

      if (ia16_insn_is_reset_ds_p (insn))
	{
	  /* Rewrite
		(set (reg:SEG DS_REG) (reg:SEG SS_REG))
	     as
		(use (reg:SEG SS_REG)).

	     If this insn is followed by
		(use (reg:SEG DS_REG))
	     then rewrite that too.  */
	  rtx ss = XEXP (PATTERN (insn), 1);
	  PATTERN (insn) = gen_rtx_USE (VOIDmode, ss);
	  INSN_CODE (insn) = -1;

	  if (insn != BB_END (BLOCK_FOR_INSN (insn)))
	    {
	      rtx_insn *next_insn = NEXT_INSN (insn);
	      if (ia16_insn_is_use_ds_p (next_insn))
		XEXP (PATTERN (next_insn), 0) = ss;
	    }

	  continue;
	}

      if (! NONJUMP_INSN_P (insn) || GET_CODE (PATTERN (insn)) != SET)
	continue;

      pat = PATTERN (insn);
      dest = SET_DEST (pat);
      src = SET_SRC (pat);

      if (MEM_P (dest)
	  && ADDR_SPACE_GENERIC_P (MEM_ADDR_SPACE (dest))
	  && (REG_P (src) || CONSTANT_P (src)))
	{
	  /* If the insn is a simple move to generic memory, and the address
	     does not involve %bp, then _add_ a `%ds:' segment override to
	     the destination address term.

	     (These added overrides are not counted towards determining
	      whether %ds is ever live (below).  As Obi-Wan never said:
	      "These are not the %ds references you are looking for.")  */
	  rtx addr = XEXP (dest, 0), rb, rs;

	  if (ia16_parse_address_strict (addr, &rb, NULL, NULL, &rs)
	      && ! rs && (! rb || REGNO (rb) != BP_REG))
	    {
	      rtx new_dest = shallow_copy_rtx (dest);
	      XEXP (new_dest, 0) = gen_rtx_PLUS (HImode, override, addr);
	      PATTERN (insn) = gen_rtx_SET (new_dest, src);
	    }
	}
      else if (MEM_P (src)
	       && ADDR_SPACE_GENERIC_P (MEM_ADDR_SPACE (src))
	       && REG_P (dest))
	{
	  /* Ditto if the insn is a simple move from generic memory.  */
	  rtx addr = XEXP (src, 0), rb, rs;

	  if (ia16_parse_address_strict (addr, &rb, NULL, NULL, &rs)
	      && ! rs && (! rb || REGNO (rb) != BP_REG))
	    {
	      rtx new_src = shallow_copy_rtx (src);
	      XEXP (new_src, 0) = gen_rtx_PLUS (HImode, override, addr);
	      PATTERN (insn) = gen_rtx_SET (dest, new_src);
	    }
	}
    }

  /* If at no point in the function do we need to reset %ds to .data, then
     deduce that %ds is no longer live within the function.  */
  if (default_ds_abi_p && ! keep_any_resets_p)
    df_set_regs_ever_live (DS_REG, false);

  free (insn_pushed_p);
  free (keep_insn_p);
  free (stack);
}

#undef	TARGET_MACHINE_DEPENDENT_REORG
#define	TARGET_MACHINE_DEPENDENT_REORG	ia16_machine_dependent_reorg

static void
ia16_machine_dependent_reorg (void)
{
  ia16_verify_calls_from_no_assume_ds ();
  ia16_verify_calls_from_far_section ();

  if (optimize && TARGET_ALLOCABLE_DS_REG && call_used_regs[DS_REG])
    {
      compute_bb_for_insn ();
      ia16_elide_unneeded_ss_stuff ();
      free_bb_for_insn ();
    }
}

enum ia16_builtin
{
  IA16_BUILTIN_SELECTOR,
  IA16_BUILTIN_FP_OFF,
  IA16_BUILTIN_MAX
};

static GTY (()) tree ia16_builtin_decls[IA16_BUILTIN_MAX];

#undef	TARGET_INIT_BUILTINS
#define	TARGET_INIT_BUILTINS	ia16_init_builtins

static void
ia16_init_builtins (void)
{
  tree intSEG_type_node = unsigned_intHI_type_node, const_void_far_type_node,
       const_void_far_ptr_type_node,
       intSEG_ftype_intHI, unsigned_intHI_ftype_const_void_far_ptr, func;

  /* With -mprotected-mode, we need a type node for PHImode for use with
     __builtin_ia16_selector (), but there is no such existing node.  So
     fashion one.  */
  if (TARGET_PROTECTED_MODE)
    {
      intSEG_type_node = build_distinct_type_copy (unsigned_intHI_type_node);
      SET_TYPE_MODE (intSEG_type_node, PHImode);
    }
  (*lang_hooks.types.register_builtin_type) (intSEG_type_node,
					     "__builtin_ia16_segment_t");
  const_void_far_type_node
    = build_qualified_type (void_type_node,
			    TYPE_QUAL_CONST
			    | ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
  const_void_far_ptr_type_node = build_pointer_type (const_void_far_type_node);

  intSEG_ftype_intHI = build_function_type_list (intSEG_type_node,
						 unsigned_intHI_type_node,
						 NULL_TREE);
  unsigned_intHI_ftype_const_void_far_ptr
    = build_function_type_list (unsigned_intHI_type_node,
				const_void_far_ptr_type_node, NULL_TREE);

  func = add_builtin_function ("__builtin_ia16_selector", intSEG_ftype_intHI,
			       IA16_BUILTIN_SELECTOR, BUILT_IN_MD, NULL,
			       NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_SELECTOR] = func;

  func = add_builtin_function ("__builtin_ia16_FP_OFF",
			       unsigned_intHI_ftype_const_void_far_ptr,
			       IA16_BUILTIN_FP_OFF, BUILT_IN_MD, NULL,
			       NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_FP_OFF] = func;
}

#undef	TARGET_BUILTIN_DECL
#define	TARGET_BUILTIN_DECL	ia16_builtin_decl

static tree
ia16_builtin_decl (unsigned code, bool initialize_p ATTRIBUTE_UNUSED)
{
  if (code < IA16_BUILTIN_MAX)
    return ia16_builtin_decls[code];
  else
    return error_mark_node;
}

#undef	TARGET_EXPAND_BUILTIN
#define	TARGET_EXPAND_BUILTIN	ia16_expand_builtin

static rtx
ia16_expand_builtin (tree expr, rtx target ATTRIBUTE_UNUSED,
		     rtx subtarget ATTRIBUTE_UNUSED,
		     machine_mode mode ATTRIBUTE_UNUSED,
		     int ignore ATTRIBUTE_UNUSED)
{
  tree fndecl = TREE_OPERAND (CALL_EXPR_FN (expr), 0), arg0;
  rtx op0;
  unsigned fcode = DECL_FUNCTION_CODE (fndecl);

  switch (fcode)
    {
    case IA16_BUILTIN_SELECTOR:
      arg0 = CALL_EXPR_ARG (expr, 0);
      op0 = expand_normal (arg0);
      return ia16_bless_selector (op0);

    case IA16_BUILTIN_FP_OFF:
      arg0 = CALL_EXPR_ARG (expr, 0);
      op0 = expand_normal (arg0);
      return ia16_far_pointer_offset (op0);

    default:
      gcc_unreachable ();
    }
}

static bool
ia16_fp_off_foldable_for_as (addr_space_t as)
{
  return as == ADDR_SPACE_FAR || as == ADDR_SPACE_GENERIC;
}

#undef	TARGET_FOLD_BUILTIN
#define	TARGET_FOLD_BUILTIN	ia16_fold_builtin

static tree
ia16_fold_builtin (tree fndecl, int n_args, tree *args,
		   bool ignore ATTRIBUTE_UNUSED)
{
  static tree ptrtype = NULL_TREE;
  unsigned fcode = DECL_FUNCTION_CODE (fndecl);
  tree op, fake;

  switch (fcode)
    {
    case IA16_BUILTIN_SELECTOR:
      gcc_assert (n_args == 1);
      /* If -mprotected-mode is off, `__builtin_ia16_selector (seg)' is the
	 same as `seg', so we can fold the call right here.  If not, pass
	 the buck to ia16_expand_builtin (...) above.  */
      if (TARGET_PROTECTED_MODE)
	return NULL_TREE;
      return args[0];

    case IA16_BUILTIN_FP_OFF:
      if (flag_lto)
	return NULL_TREE;

      gcc_assert (n_args == 1);
      /* Do a hack to handle the special case of using a far pointer's
	 offset in a static initializer, like so:
		void foo (void) __far;
		static unsigned bar = __builtin_ia16_FP_OFF (foo);
	 The FreeDOS kernel code does something like the above when installing
	 its interrupt handlers.  */
      op = args[0];

      if (! TREE_CONSTANT (op))
	return NULL_TREE;

      /* Check if we are taking the address of a public or external near/far
	 function or near/far variable; if not, again hand over to
	 ia16_expand_builtin (...).  First strip away layers of NOP_EXPR.

	 TODO: generalize this to handle other types of far addresses.  And,
	 get this feature to work with LTO.  */
      while (TREE_CODE (op) == NOP_EXPR
	     && POINTER_TYPE_P (TREE_TYPE (op))
	     && ia16_fp_off_foldable_for_as (TYPE_ADDR_SPACE
					     (TREE_TYPE (TREE_TYPE (op)))))
	op = TREE_OPERAND (op, 0);

      if (TREE_CODE (op) != ADDR_EXPR
	  || ! POINTER_TYPE_P (TREE_TYPE (op))
	  || ! ia16_fp_off_foldable_for_as (TYPE_ADDR_SPACE
					    (TREE_TYPE (TREE_TYPE (op)))))
	return NULL_TREE;

      op = TREE_OPERAND (op, 0);
      if (! VAR_OR_FUNCTION_DECL_P (op))
	return NULL_TREE;
      if (! DECL_EXTERNAL (op) && ! TREE_PUBLIC (op))
	return NULL_TREE;

      /* Mark the original declaration as addressable.  */
      mark_addressable (op);

      /* Create a fake external "declaration" which has the same assembler
	 name, but which is placed in the generic address space.  Then take
	 the address of that.  Mark the pointer as artificial, and as being
	 able to alias anything.  */
      fake = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			 create_tmp_var_name (NULL), char_type_node);
      SET_DECL_ASSEMBLER_NAME (fake, DECL_ASSEMBLER_NAME (op));
      DECL_EXTERNAL (fake) = 1;
      DECL_ARTIFICIAL (fake) = 1;

      if (! ptrtype)
	ptrtype = build_pointer_type_for_mode (char_type_node, HImode, true);

      mark_addressable (fake);
      op = build_fold_addr_expr_with_type_loc (UNKNOWN_LOCATION, fake,
					       ptrtype);
      op = fold_build1 (NOP_EXPR, unsigned_intHI_type_node, op);
      return op;

    default:
      gcc_unreachable ();
    }
}

/* The Global targetm Variable */

/* #include "target.h" */
/* #include "target-def.h" */

/* Initialize the GCC target structure.  */
struct gcc_target targetm = TARGET_INITIALIZER;

/* Support functions not documented in the GCC internals manual.  */

/* Return true if the operands are suitable for an insn of code CODE.
 * Return false otherwise.  Note that we don't bother checking combinations
 * of register and immediate operands since the operand predicates should be
 * enough.  SCRATCH expressions are also accepted for operand 0.
 * The insn is assumed to have exactly three operands.
 */
bool ia16_arith_operands_p (enum rtx_code code, rtx *operands)
{
  if (MEM_P (operands[1]) && MEM_P (operands[2]))
    return (false);

  if (MEM_P (operands[0]))
    {
      /* OK if op0 and op1 is the same MEM.  */
      if (rtx_equal_p (operands[0], operands[1]))
	return (true);

      /* OK if op0 and op2 is the same MEM if
	 1) CODE is commutative, or
	 2) CODE is MINUS and op1 is zero.  */
      if ((RTX_COMM_ARITH == GET_RTX_CLASS (code)
	   || (MINUS == code
	      && operands[1] == CONST0_RTX (GET_MODE (operands[1]))))
	  && rtx_equal_p (operands[0], operands[2]))
	return (true);
    }
  else if ((RTX_COMM_ARITH == GET_RTX_CLASS (code)
	    && !CONSTANT_P (operands[2]))
	   || !MEM_P (operands[1]))
    return (true);
  return (false);
}

/* Mung OPERANDS enough to be accepted by ia16_arith_operands_p (CODE,
   OPERANDS).  If operands[0] needs to be munged, return a new pseudo register
   operand to use as a temporary.  Otherwise, return NULL.
   This function may create new pseudo registers.
   TODO: Three different memory operands are all forced to registers.  Ideally,
   only operands[0] and operands[1] should be forced to registers.
 */
rtx ia16_prepare_operands (enum rtx_code code, rtx *operands)
{
  rtx tmp = NULL;

  /* If op1 is MEM, op0 must be the same mem.  If they are the same mem, then
     op2 must not also be a mem.  */
  if (MEM_P (operands[1]))
    {
      if (!rtx_equal_p (operands[0], operands[1]))
	operands[1] = force_reg (GET_MODE (operands[1]), operands[1]);
      else if (MEM_P (operands[2]))
	operands[2] = force_reg (GET_MODE (operands[2]), operands[2]);
    }

  /* If op2 is MEM, op0 must be the same mem and CODE != MINUS.  */
  if (MEM_P (operands[2])
      && (RTX_BIN_ARITH == GET_RTX_CLASS (code)
	  || !rtx_equal_p (operands[0], operands[2])))
    operands[2] = force_reg (GET_MODE (operands[2]), operands[2]);

  /* If op0 is MEM different from op1 and op2, use a scratch register.  */
  if (MEM_P (operands[0])
      && (!MEM_P (operands[1]) || !MEM_P (operands[2])))
    tmp = gen_reg_rtx (GET_MODE (operands[0]));
  return (tmp);
}

/* Split a memory address displacement X into a symbolic part SYMVAL and an
 * integer part INTVAL.  X may be NULL.
 */
static void
ia16_parse_constant (rtx x, rtx *symval, rtx *intval)
{
  if (x == NULL)
    {
      *symval = NULL;
      *intval = const0_rtx;
    }
  else if (CONST_INT_P (x))
    {
      *symval = NULL;
      *intval = x;
    }
  /* Try to handle (const (plus (...) (const_int c))).  */
  else if (CONST == GET_CODE (x) && PLUS == GET_CODE (XEXP (x, 0)))
    {
      if (CONST_INT_P (XEXP (XEXP (x, 0), 1)))
	{
	  *symval = XEXP (XEXP (x, 0), 0);
	  *intval = XEXP (XEXP (x, 0), 1);
	}
      else
	{
	  *symval = XEXP (x, 0);
	  *intval = const0_rtx;
	}
    }
  else
    {
      *symval = x;
      *intval = const0_rtx;
    }
}

/* Return true if the addresses of memory operands M1 and M2 differ only by
 * a fixed, numeric value, and store the number of units of M1 - M2 in OFFSET.
 * Return false otherwise.  */
static bool
ia16_memory_offset_known (rtx m1, rtx m2, int *offset)
{
  rtx m1base, m1index, m1disp, m1seg, m1sym, m1int;
  rtx m2base, m2index, m2disp, m2seg, m2sym, m2int;

  ia16_parse_address_strict (XEXP (m1, 0), &m1base, &m1index, &m1disp, &m1seg);
  ia16_parse_address_strict (XEXP (m2, 0), &m2base, &m2index, &m2disp, &m2seg);

  if (!rtx_equal_p (m1base, m2base) || !rtx_equal_p (m1index, m2index) ||
      !rtx_equal_p (m1seg, m2seg))
    return (false);

  ia16_parse_constant (m1disp, &m1sym, &m1int);
  ia16_parse_constant (m2disp, &m2sym, &m2int);

  if (!rtx_equal_p (m1sym, m2sym))
    return (false);

  *offset = INTVAL (m1int) - INTVAL (m2int);
  return (true);
}

/* Return true if memory operands M1 and M2 are suitable for a move
 * multiple instruction in mode MODE.  Return false otherwise.  */
bool
ia16_move_multiple_mem_p (enum machine_mode mode, rtx m1, rtx m2)
{
  int offset;

  if (ia16_memory_offset_known (m2, m1, &offset))
    return (GET_MODE_SIZE (mode) == offset);
  else
    return (false);
}

/* Return true if register operands R1 and R2 are suitable for a move
 * multiple instruction in mode MODE.  Return false otherwise.  */
bool
ia16_move_multiple_reg_p (enum machine_mode mode, rtx r1, rtx r2)
{
  enum machine_mode mode2x = GET_MODE_2XWIDER_MODE (mode);
  unsigned int reg1no = REGNO (r1);

  if (!HARD_REGNO_MODE_OK (reg1no, mode2x))
    return (false);
  return (REGNO (r2) - reg1no
	  == subreg_regno_offset (reg1no, mode2x, GET_MODE_SIZE (mode), mode));
}

/* Return true if we can prove that memory operands M1 and M2 don't overlap.
 * Return false otherwise.  */
bool
ia16_non_overlapping_mem_p (rtx m1, rtx m2)
{
  int offset;

  if (!ia16_memory_offset_known (m1, m2, &offset))
    return (false);

  if (offset < 0)
    return (GET_MODE_SIZE (GET_MODE (m1)) <= -offset);
  else if (offset > 0)
    return (GET_MODE_SIZE (GET_MODE (m2)) <=  offset);
  else
    return (false);
}

/* Emit instructions to reset %ds to .data.  REGNO is either a scratch
   register number, or a large number.  */
static void
ia16_expand_reset_ds_internal (void)
{
  if (TARGET_PROTECTED_MODE)
    emit_insn (gen__reset_ds_slow_prot_mode ());
  else
    emit_insn (gen__reset_ds_slow ());
  emit_use (gen_rtx_REG (SEGmode, DS_REG));
}

void
ia16_expand_reset_ds_for_call (rtx addr)
{
  if (ia16_default_ds_abi_function_rtx_p (addr))
    ia16_expand_reset_ds_internal ();
}

void
ia16_expand_reset_ds_for_return (void)
{
  if (ia16_in_default_ds_abi_function_p ())
    ia16_expand_reset_ds_internal ();
}

void
ia16_expand_prologue (void)
{
  rtx insn;
  unsigned int i;
  HOST_WIDE_INT size = get_frame_size ();

  /* Save used registers which are not call clobbered. */
  for (i = 0; i <= LAST_ALLOCABLE_REG; ++i)
    {
      if (i != BP_REG && ia16_save_reg_p (i))
	{
	  insn = emit_insn (ia16_push_reg (i));
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
    }
  if (ia16_save_reg_p (BP_REG))
    {
      if (HAVE__enter && size != 0)
	{
	  insn = gen__enter (gen_rtx_CONST_INT (HImode, size + 2));
	  insn = emit_insn (insn);
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
      else
	{
	  insn = emit_insn (ia16_push_reg (BP_REG));
	  RTX_FRAME_RELATED_P (insn) = 1;
	  insn = emit_move_insn (gen_rtx_REG (Pmode, BP_REG),
				 gen_rtx_REG (Pmode, SP_REG));
	  RTX_FRAME_RELATED_P (insn) = 1;
	  if (size != 0)
	    {
	      insn = emit_insn (gen_subhi3 (gen_rtx_REG (Pmode, SP_REG),
					    gen_rtx_REG (Pmode, SP_REG),
					    gen_rtx_CONST_INT (HImode, size)));
	      RTX_FRAME_RELATED_P (insn) = 1;
	    }
	}
    }
  if (flag_stack_usage_info)
    {
      current_function_static_stack_size = size +
	ia16_initial_arg_pointer_offset () + UNITS_PER_WORD;
    }
}

void
ia16_expand_epilogue (bool sibcall)
{
  unsigned int i;
  HOST_WIDE_INT size = get_frame_size ();

  if (ia16_save_reg_p (BP_REG))
    {
      /* We need to restore the stack pointer. We have two options:
       * 1) Add the frame size to sp.
       * 2) Use the saved sp value in bp.
       * The second option makes it possible to use the leave instruction and
       * is shorter in any case, but ties up a register. So only refer to bp
       * if it wasn't used during the function.
       */
      if (HAVE__leave)
	{
	  /* If sp wasn't modified, "popw %bp" is faster than "leavew". */
	  if (size == 0 && crtl->sp_is_unchanging)
	    emit_insn (ia16_pop_reg (BP_REG));
	  else
	    emit_insn (gen__leave ());
	}
      else
	{
	  if (EXIT_IGNORE_STACK)
	    {
	      emit_move_insn (gen_rtx_REG (Pmode, SP_REG),
			      gen_rtx_REG (Pmode, BP_REG));
	    }
	  emit_insn (ia16_pop_reg (BP_REG));
	}
    }
  /* Restore used registers. */
  for (i = LAST_ALLOCABLE_REG; i < FIRST_PSEUDO_REGISTER; --i)
    {
      if (i != BP_REG && ia16_save_reg_p (i))
	emit_insn (ia16_pop_reg (i));
    }
  if (!sibcall)
    emit_jump_insn (gen_simple_return ());
}

/* Return the insn template for a normal call to a subroutine at address
   ADDR and with machine mode MODE.  ADDR should correspond to "%0", "%1",
   or "%3" in the (define_insn ...), depending on whether WHICH_IS_ADDR is
   0, 1, or 3.

   There are 4 cases we need to handle:
     * we are calling a near function (with a 16-bit address)
     * we are calling a far function through a 32-bit pointer
     * we are calling a far function --- which returns with `lret' --- but
       the function is defined (in the default text section) in the current
       module, and is not marked __attribute__ ((far_section)), so we can get
       away with a `pushw %cs' plus a near call
     * none of the above --- use `lcall' and do not assume that the function
       resides in the default text section.  */
#define P(part)	(which_is_addr == 0 ? part "0" : \
		 which_is_addr == 1 ? part "1" : \
				      part "3")
#define P2(part_a, part_b) \
			(which_is_addr == 0 ? part_a "0" part_b "0" : \
			 which_is_addr == 1 ? part_a "1" part_b "1" : \
					      part_a "3" part_b "3")
const char *
ia16_get_call_expansion (rtx addr, machine_mode mode, unsigned which_is_addr)
{
  tree fndecl, fntype = NULL_TREE;

  if (mode == SImode)
    {
      if (CONST_INT_P (addr))
	return P2 ("lcall\t%S", ",\t%O");
      else
	return P ("lcall\t*%");
    }

  fndecl = ia16_get_function_decl_for_addr (addr);
  if (fndecl)
    {
      fntype = TREE_TYPE (fndecl);
      while (POINTER_TYPE_P (fntype))
	fntype = TREE_TYPE (fntype);
    }

  if (! fntype || TYPE_ADDR_SPACE (fntype) != ADDR_SPACE_FAR)
    {
      if (MEM_P (addr) || ! CONSTANT_P (addr))
	return P ("call\t*%");
      else
	return P ("call\t%c");
    }
  else if ((DECL_INITIAL (fndecl)
	    && ! ia16_far_section_function_type_p (fntype))
	   || ia16_near_section_function_type_p (fntype))
    {
      if (MEM_P (addr) || ! CONSTANT_P (addr))
	return P ("pushw\t%%cs\n\tcall\t*%");
      else
	return P ("pushw\t%%cs\n\tcall\t%c");
    }
  else
    {
      if (! TARGET_SEG_RELOC_STUFF)
	ia16_error_seg_reloc (input_location, "cannot create relocatable call "
					      "to far function");
      /* GNU as does not like
		lcall	$foo@SEGMENT16,	$foo
	 and says "Error: can't handle non absolute segment in `lcall'".  */
      return P2 (  ".reloc\t.+3, R_386_SEGMENT16, %c", "\n"
		 "\tlcall\t$0,\t%");
    }
}

/* Return the insn template for a sibling call to a subroutine at address
   ADDR and with machine mode MODE.  ADDR should correspond to "%0", "%1",
   or "%3" in the (define_insn ...), depending on whether WHICH_IS_ADDR is
   0, 1, or 3.  */
const char *
ia16_get_sibcall_expansion (rtx addr, machine_mode mode,
			    unsigned which_is_addr)
{
  tree fndecl, fntype = NULL_TREE;

  if (mode == SImode)
    {
      if (CONST_INT_P (addr))
	return P2 ("ljmp\t%S", ",\t%O");
      else
	return P ("ljmp\t*%");
    }

  fndecl = ia16_get_function_decl_for_addr (addr);
  if (fndecl)
    {
      fntype = TREE_TYPE (fndecl);
      while (POINTER_TYPE_P (fntype))
	fntype = TREE_TYPE (fntype);
    }

  if (! fntype || TYPE_ADDR_SPACE (fntype) != ADDR_SPACE_FAR)
    {
      if (MEM_P (addr) || ! CONSTANT_P (addr))
	return P ("jmp\t*%");
      else
	return P ("jmp\t%c");
    }
  else if (DECL_INITIAL (fndecl)
	   || ia16_near_section_function_type_p (fntype))
    {
      if (MEM_P (addr) || ! CONSTANT_P (addr))
	return P ("jmp\t*%");
      else
	return P ("jmp\t%c");
    }
  else
    {
      if (! TARGET_SEG_RELOC_STUFF)
	ia16_error_seg_reloc (input_location, "cannot create relocatable "
					      "sibcall to far function");
      return P2 (  ".reloc\t.+3, R_386_SEGMENT16, %c", "\n"
		 "\tljmp\t$0,\t%");
    }
}

#undef P
#undef P2

void
ia16_asm_output_addr_diff_elt (FILE *stream, rtx body ATTRIBUTE_UNUSED,
			   int value, int rel)
{
  asm_fprintf (stream, "\t.hword\t%U%LL%d-%U%LL%d\n", value, rel);
}

void
ia16_asm_output_addr_vec_elt (FILE *stream, int value)
{
  asm_fprintf (stream, "\t.hword\t%U%LL%d\n", value);
}

#include "gt-ia16.h"
