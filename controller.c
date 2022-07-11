#include <libpad.h>
#include <stdio.h>
#include <loadfile.h>
#include <sifrpc.h>

#define CONT_PORT 0
#define CONT_SLOT 0

#define PAD_READY_STATE 6

static char padBuf[256] __attribute__((aligned(64)));

static char actAlign[6];
static int actuators;

struct pad_data
{
    struct padButtonStatus buttons;
    u32 data;
    u32 old_pad;
    u32 new_pad;
};

// thanks to pukko

/*
 * Local functions
 */

/*
 * loadModules()
 */
static void
loadModules(void)
{
    int ret;

    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0)
    {
        printf("sifLoadModule sio failed: %d\n", ret);
        SleepThread();
    }

    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0)
    {
        printf("sifLoadModule pad failed: %d\n", ret);
        SleepThread();
    }
}

/*
 * waitPadReady()
 */
static int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;
    while ((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
    {
        if (state != lastState)
        {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n",
                   port, slot, stateString);
        }
        lastState = state;
        state = padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1)
    {
        printf("Pad OK!\n");
    }
    return 0;
}

/*
 * initializePad()
 */
static int
initializePad(int port, int slot)
{

    int ret;
    int modes;
    int i;

    waitPadReady(port, slot);

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0)
    {
        printf("( ");
        for (i = 0; i < modes; i++)
        {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n",
           padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller
    // (it has no actuator engines)
    if (modes == 0)
    {
        printf("This is a digital controller?\n");
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do
    {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes)
    {
        printf("This is no Dual Shock controller\n");
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0)
    {
        printf("This is no Dual Shock controller??\n");
        return 1;
    }

    printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

    waitPadReady(port, slot);
    actuators = padInfoAct(port, slot, -1, 0);
    printf("# of actuators: %d\n", actuators);

    if (actuators != 0)
    {
        actAlign[0] = 0; // Enable small engine
        actAlign[1] = 1; // Enable big engine
        actAlign[2] = 0xff;
        actAlign[3] = 0xff;
        actAlign[4] = 0xff;
        actAlign[5] = 0xff;

        waitPadReady(port, slot);
        printf("padSetActAlign: %d\n",
               padSetActAlign(port, slot, actAlign));
    }
    else
    {
        printf("Did not find any actuators.\n");
    }

    waitPadReady(port, slot);

    return 1;
}

void init_controller()
{
    SifInitRpc(0);

    loadModules();

    int ret;

    SifInitRpc(0);
    loadModules();
    padInit(0);

    int port, slot;

    port = 0; // 0 -> Connector 1, 1 -> Connector 2
    slot = 0; // Always zero if not using multitap
    if ((ret = padPortOpen(port, slot, padBuf)) == 0)
    {
        printf("padOpenPort failed: %d\n", ret);
        SleepThread();
    }

    if (!initializePad(port, slot))
    {
        printf("pad initalization failed!\n");
        SleepThread();
    }
}

void update_pad(struct pad_data *pad)
{
    padRead(0, 0, &pad->buttons);
    unsigned short data = 0xffff ^ pad->buttons.btns;
    pad->new_pad = data & ~pad->old_pad;
    pad->old_pad = data;
    pad->data = data;
}

// returns whether the given button(s) is/are currently held
int pad_btn_held(struct pad_data *pad, u32 btn_mask)
{
    return !!(pad->data & btn_mask);
}

// returns whether this button was pressed this update cycle
int pad_btn_pressed(struct pad_data *pad, u32 btn_mask)
{
    return !!(pad->new_pad & btn_mask);
}

#define ANALOG_STICK_RIGHT 0
#define ANALOG_STICK_LEFT 1

struct analog_output
{
    float horizontal;
    float vertical;
};

// -1 for
struct analog_output read_analog_stick(struct pad_data *pad, int stick)
{
    int v, h;
    v = h = 0.0;
    switch (stick)
    {
    case ANALOG_STICK_LEFT:
        v = pad->buttons.ljoy_v;
        h = pad->buttons.ljoy_h;
        break;
    case ANALOG_STICK_RIGHT:
        v = pad->buttons.rjoy_v;
        h = pad->buttons.rjoy_h;
        break;
    }

    v -= 128;
    h -= 128;
    float vf, hf;
    vf = v / 128.0f;
    hf = h / 128.0f;
    return (struct analog_output){hf, vf};
}