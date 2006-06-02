/*---------------------------------------------------------------------------------
	$Id: main.c,v 1.11 2006-06-02 11:14:18 wntrmute Exp $

	Simple ARM7 stub (sends RTC, TSC, and X/Y data to the ARM 9)

	$Log: not supported by cvs2svn $
	Revision 1.10  2005/10/19 16:30:44  wntrmute
	updated default arm7 core
	bumped ndstool version
	
	Revision 1.9  2005/09/13 03:13:36  wntrmute
	rework to use proper interrupt dispatcher
	
	Revision 1.8  2005/08/03 05:13:16  wntrmute
	corrected sound code


---------------------------------------------------------------------------------*/
#include <nds.h>
 
//---------------------------------------------------------------------------------
void startSound(	int sampleRate,
					const void* data, u32 bytes,
					u8 channel, u8 vol, u8 format) {
//---------------------------------------------------------------------------------
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | (format==1?SOUND_8BIT:SOUND_16BIT);
}
 
 
//---------------------------------------------------------------------------------
s32 getFreeSoundChannel() {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}
 
 
//---------------------------------------------------------------------------------
void irqVblank() {
//---------------------------------------------------------------------------------
	static int heartbeat = 0;
 
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt=0;
	int t1=0, t2=0;
	uint32 temp=0;
	uint8 ct[sizeof(IPC->curtime)];
	u32 i;
 
	// Update the heartbeat
	heartbeat++;
 
	// Read the touch screen
 
	but = REG_KEYXY;
 
	if (!(but & (1<<6))) {
 
		touchPosition tempPos = touchReadXY();

		x = tempPos.x;
		y = tempPos.y;
		xpx = tempPos.px;
		ypx = tempPos.py;
		z1 = tempPos.z1;
		z2 = tempPos.z2;
		
	}
 
	batt = touchRead(TSC_MEASURE_BATTERY);
 
	// Read the time
	rtcGetTime((uint8 *)ct);
	BCDToInteger((uint8 *)&(ct[1]), 7);
 
	// Read the temperature
	temp = touchReadTemperature(&t1, &t2);
 
	IPC->mailBusy = 1;
	// Update the IPC struct
	IPC->heartbeat	= heartbeat;
	IPC->buttons		= but;
	IPC->touchX			= x;
	IPC->touchY			= y;
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->battery		= batt;
	IPC->mailBusy = 0;
 
	for(i=0; i<sizeof(ct); i++) {
		IPC->curtime[i] = ct[i];
	}
 
	IPC->temperature = temp;
	IPC->tdiode1 = t1;
	IPC->tdiode2 = t2;
 

	//sound code  :)
	TransferSound *snd = IPC->soundData;
	IPC->soundData = 0;
 
	if (0 != snd) {
 
		for (i=0; i<snd->count; i++) {
			s32 chan = getFreeSoundChannel();
 
			if (chan >= 0) {
				startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].format);
			}
		}
	}
 
 
}
 
 
//---------------------------------------------------------------------------------
// timer 0 irq handler
//---------------------------------------------------------------------------------
void irqTimer0(void) {
//---------------------------------------------------------------------------------
	ProcessMicrophoneTimerIRQ();
}
 
 
//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------
	// reset the clock if needed
	rtcReset();
 
	// enable sound
	powerON(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7f);
	IPC->soundData = 0;
	IPC->mailBusy = 0;
 
	// setup irq
	irqInit();
 
	irqSet(IRQ_VBLANK, irqVblank);
	irqSet(IRQ_TIMER0, irqTimer0);
	irqEnable(IRQ_VBLANK | IRQ_TIMER0);
 
	// keep the ARM7 out of main RAM
	while (1) swiWaitForVBlank();
}

