#ifndef _BUZZER_H_
#define _BUZZER_H_

// Sound Type
#define BUZZ_ACT		'A'
#define BUZZ_OK	        'O'
#define BUZZ_ERROR		'E'

#define BUZZ_STOP       'S'
#define BUZZ_WAIT       'W'
#define BUZZ_DONE       'D'

#define BUZZ_CONNECT	'C'
#define BUZZ_TRANSFER   'T'
#define BUZZ_RECEIVE	'R'

#define BUZZ_SINGLE	'G'
#define BUZZ_VALUE	'V'

int play_sound(int sound_type, int val);


// Tune
#define ERROR		BUZZ_ERROR
#define SUCCESS		BUZZ_OK
#define CONNECTION	BUZZ_CONNECT
#define SENDING		BUZZ_TRANSFER
#define RECEIVING	BUZZ_RECEIVE
#define STOP_BUZZER         BUZZ_STOP
#define FINISH              BUZZ_DONE
#define WAITING             BUZZ_WAIT
#define PRODUCTION_TEST     BUZZ_ACT
#define SHAKE_DETECT        BUZZ_ACT

//#define setTune(a, b); play_sound(a, b);

#endif
