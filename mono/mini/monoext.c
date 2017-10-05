/**
 * \file
 * support for monoext, based on simd-intrinsics.c
 *
 * Author:
 *   Jay Krell (jaykrell@microsoft.com)
 *
 * NOTES:
 * add -> add_imm, shift -> shift_imm optimizations
 * often occur at the CEE => OP level (IL to IR).
 * As such, this code becomes perhaps the appropriate
 * place for such optimizations, or IR optimization.
 */


#include <config.h>
#include "mini.h"
#include "ir-emit.h"

#ifdef MONO_ARCH_MONOEXT

static void mono_ext_coverage(guint line);
#define MONO_EXT_COVERAGE() mono_ext_coverage(__LINE__)

typedef struct _MonoExtFunction {
    // matching
	guint32 opt;
    const char* name;
    MonoTypeEnum return_type;
    guint8 param_count;
    MonoTypeEnum param_types[2];

    // IR
    guint16 opcode;
    guint8 stack_type;

    guint coverage_line;
} MonoExtFunction;

static const MonoExtFunction MonoExtFunctions[] = {
	// opt							name                    return_type     param_count	param_types                     opcode                      stack_type    coverage_line
	// TODO There are no current IL-based or C-based fallbacks break/rdtsc so they justify different enablement.
	{ 0,							"DebugBreak",			MONO_TYPE_VOID, 0,			{ MONO_TYPE_END },              OP_BREAK,                   STACK_VTYPE,  __LINE__ },
    { 0,							"ReadTimeStampCounter",	MONO_TYPE_U8,   0,			{ MONO_TYPE_END },              OP_READ_TIME_STAMP_COUNTER, STACK_I8,     __LINE__ },
#ifdef MONO_ARCH_ROTATE_INTRINSICS
	// second parameter for rotate is I4 to be like shift
    { MONO_OPT_ROTATE_INTRINSICS,	"RotateLeft",			MONO_TYPE_U4,   2,			{ MONO_TYPE_U4, MONO_TYPE_I4 }, OP_IROL,                    STACK_I4,     __LINE__ },
    { MONO_OPT_ROTATE_INTRINSICS,	"RotateRight",			MONO_TYPE_U4,   2,			{ MONO_TYPE_U4, MONO_TYPE_I4 }, OP_IROR,                    STACK_I4,     __LINE__ },
    { MONO_OPT_ROTATE_INTRINSICS,	"RotateLeft",			MONO_TYPE_U8,   2,			{ MONO_TYPE_U8, MONO_TYPE_I4 }, OP_LROL,                    STACK_I8,     __LINE__ },
    { MONO_OPT_ROTATE_INTRINSICS,	"RotateRight",			MONO_TYPE_U8,   2,			{ MONO_TYPE_U8, MONO_TYPE_I4 }, OP_LROR,                    STACK_I8,     __LINE__ },
#endif
};

static
const MonoExtFunction*
mono_match_monoext (MonoCompile *cfg, MonoMethod *cmethod, MonoMethodSignature *fsig, MonoInst **args)
{
	MonoExtFunction const * function;
	guint i;

    if (strcmp(cmethod->klass->image->assembly->aname.name, "Mono.Ext"))
        return NULL;

    if (strcmp(cmethod->klass->name, "MonoExt"))
        return NULL;

    for (function = MonoExtFunctions; function < *(&MonoExtFunctions + 1); ++function) {
		// Enabled/disabled on command line?
		if (function->opt && (function->opt & cfg->opt) == 0)
			continue;

        if (function->return_type != fsig->ret->type ||
            function->param_count != fsig->param_count ||
            strcmp(cmethod->name, function->name))
            continue;

        for (i = 0; i < fsig->param_count; ++i)
            if (function->param_types[i] != fsig->params[i]->type)
                break;

        if (i == fsig->param_count)
            return function;
	}

    return NULL;
}

#endif // MONO_ARCH_MONOEXT

MonoInst*
mono_handle_monoext (MonoCompile *cfg, MonoMethod *cmethod, MonoMethodSignature *fsig, MonoInst **args)
{
#ifndef MONO_ARCH_MONOEXT

	// Consider decomposition of rotate into shifts at this level.

    return NULL;

#else

    MonoInst* ins;
    MonoExtFunction const * const function = mono_match_monoext(cfg, cmethod, fsig, args);

    if (!function)
        return NULL;

    mono_ext_coverage(function->coverage_line);

    if (cfg->verbose_level)
		printf("mono_handle_monoext assembly:%s class:%s function:%s ret:%s nargs:%d arg0:%s arg1:%s\n",
			cmethod->klass->image->assembly->aname.name,
			cmethod->klass->name,
			cmethod->name,
			cvtMonoType(fsig->ret->type),
			fsig->param_count,
			(fsig->param_count > 0) ? cvtMonoType(fsig->params[0]->type) : "",
			(fsig->param_count > 1) ? cvtMonoType(fsig->params[1]->type) : ""
			);
	
    MONO_INST_NEW (cfg, ins, function->opcode);
    ins->type = function->stack_type;
    ins->klass = cmethod->klass;

    if (fsig->ret->type != MONO_TYPE_VOID)
        ins->dreg = mono_alloc_dreg (cfg, function->stack_type);

    switch (fsig->param_count) {
    default:    g_assert_not_reached();
    case 2:     ins->sreg2 = args[1]->dreg;
                // fall through
    case 1:     ins->sreg1 = args[0]->dreg;
                // fall through
    case 0:     break;
    }

    MONO_ADD_INS (cfg->cbb, ins);

    return ins;

#endif

}

#ifdef MONO_ARCH_MONOEXT

// THIS MUST BE LATE IN THE FILE TO AVOID OVERFLOW.
static guint mono_ext_coverage_data[__LINE__];

static void mono_ext_coverage(guint line)
{
    mono_ext_coverage_data[line] += 1;
}

#endif
