#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel.h>
#include <stdio.h>

struct Library *LowLevelBase = NULL;

static const char *typename_from_state(ULONG state)
{
    ULONG type = state & JP_TYPE_MASK;
    if (type == JP_TYPE_JOYSTK) return "JOYSTICK";
    if (type == JP_TYPE_GAMECTLR) return "GAMECTLR";
#ifdef JP_TYPE_MOUSE
    if (type == JP_TYPE_MOUSE) return "MOUSE";
#endif
#ifdef JP_TYPE_UNKNOWN
    if (type == JP_TYPE_UNKNOWN) return "UNKNOWN";
#endif
#ifdef JP_TYPE_NOTAVAIL
    if (type == JP_TYPE_NOTAVAIL) return "NOTAVAIL";
#endif
    return "other";
}

static void dump_port(int port)
{
    ULONG s = ReadJoyPort(port);
    printf("port %d: state=0x%08lx type=0x%08lx (%s)\n",
           port, (unsigned long)s,
           (unsigned long)(s & JP_TYPE_MASK),
           typename_from_state(s));

    printf("  dirs: U=%d D=%d L=%d R=%d\n",
           !!(s & JPF_JOY_UP), !!(s & JPF_JOY_DOWN),
           !!(s & JPF_JOY_LEFT), !!(s & JPF_JOY_RIGHT));
    printf("  buttons: red=%d blue=%d yellow=%d green=%d forward=%d reverse=%d play=%d\n",
           !!(s & JPF_BUTTON_RED), !!(s & JPF_BUTTON_BLUE),
           !!(s & JPF_BUTTON_YELLOW), !!(s & JPF_BUTTON_GREEN),
           !!(s & JPF_BUTTON_FORWARD), !!(s & JPF_BUTTON_REVERSE),
           !!(s & JPF_BUTTON_PLAY));
}

int main(void)
{
    LowLevelBase = OpenLibrary("lowlevel.library", 40);
    if (!LowLevelBase) {
        PutStr("cannot open lowlevel.library V40+\n");
        return 1;
    }

    PutStr("Direct lowlevel.library ReadJoyPort test\n");
    dump_port(0);
    dump_port(1);
    PutStr("Move/press controller, then ENTER to sample again.\n");
    getchar();
    dump_port(0);
    dump_port(1);

    CloseLibrary(LowLevelBase);
    LowLevelBase = NULL;
    return 0;
}
