/**
 * \file
 * support for monoext, based on simd-intrinsics.c
 *
 * Author:
 *   Jay Krell (jaykrell@microsoft.com)
 *
 * (C) 2017 Microsoft, Inc.
 */

#include <config.h>
#include "mini.h"
#include "ir-emit.h"

// TODO "string recognizer" -- minimal switching or hashing,
// TODO automated signature matching

MonoInst*
mono_handle_monoext (MonoCompile *cfg, MonoMethod *cmethod, MonoMethodSignature *fsig, MonoInst **args)
{
	MonoInst* ins = NULL;

#ifdef TARGET_AMD64 // TODOROTATE more targets

	MonoClass * const klass = cmethod->klass;

	if (strcmp ("Mono.Ext", klass->image->assembly->aname.name) != 0)
        return NULL;

	if (strcmp ("MonoExt", klass->name) != 0)
        return NULL;

    char const * const name = cmethod->name;
    char const name0 = name[0];
    MonoTypeEnum const ret = fsig->ret->type;

    switch (fsig->param_count)
    {
    case 0:
        switch (ret)
        {
        case MONO_TYPE_VOID:
            if (name0 == 'D' && strcmp(cmethod->name, "DebugBreak") == 0) // Windows name
            {
	            MONO_INST_NEW (cfg, ins, OP_BREAK);
	            ins->type = STACK_VTYPE;
	            MONO_ADD_INS (cfg->cbb, ins);
            }
            break;

        case MONO_TYPE_U8:
            if (name0 == 'R' && strcmp(cmethod->name, "ReadTimeStampCounter") == 0) // Windows name
            {
	            MONO_INST_NEW (cfg, ins, OP_READ_TIME_STAMP_COUNTER);
	            ins->type = STACK_I8;
	            ins->dreg = alloc_ireg (cfg);
            }
            break;
        }
        break; // 0 params

    case 2:
        // TODO immediates
        // NOTE We could also compare the IL to known -- and skip the name/signature checks.

        if (name0 != 'R')
            return NULL;

        MonoTypeEnum const param0type = fsig->params[0]->type;

        if (ret != param0type)
            return NULL;

        if (fsig->params[1]->type != MONO_TYPE_I4)
            return NULL;

        gboolean const u4 = param0type == MONO_TYPE_U4;
        gboolean const u8 = param0type == MONO_TYPE_U8;

        if (!u4 && !u8)
            return NULL;

        size_t const len = strlen(cmethod->name);
        guint8 const stack_type = u4 ? STACK_I4 : STACK_I8;
        guint16 opcode;

        switch (len)
        {
        case sizeof("RotateLeft") - 1:
            if (strcmp(name, "RotateLeft") != 0)
                return NULL;
            opcode = u4 ? OP_IROL : OP_LROL;
            break;

        case sizeof("RotateRight") - 1:
            if (strcmp(name, "RotateRight") != 0)
                return NULL;
            opcode = u4 ? OP_IROR : OP_LROR;
            break;

        default:
            return NULL;
        }

		JK_DEBUG("1");
	    MONO_INST_NEW (cfg, ins, opcode);
	    ins->type = stack_type;
        ins->sreg1 = args [0]->dreg;
        ins->sreg2 = args [1]->dreg;
		ins->dreg = mono_alloc_dreg (cfg, stack_type);
        break;
    } // param_count

    if (ins)
    {
        printf("mono_handle_monoext handled %s\n", name);
	    ins->klass = klass;
	    MONO_ADD_INS (cfg->cbb, ins);
    }

#endif
	return ins;
}
