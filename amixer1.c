#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include <sys/poll.h>
#include <stdint.h>

int main(int argc, char *argv[])
{

	snd_ctl_t *ctlp=NULL;
	snd_ctl_elem_value_t *elemval=NULL;
	snd_ctl_elem_id_t *elemid=NULL;
	snd_ctl_elem_info_t *eleinfo=NULL;
	snd_ctl_event_t *ctlevent=NULL;
	int ret, i;
	unsigned int value;
	unsigned int count;
	struct pollfd pfds;
	unsigned short revents;
	const char *elename;
	char card_name[128];
	char *ctlname;
	snd_ctl_event_type_t event;

	if (argc < 3) {
		printf("usage: card_number, control_name\n");
		return 0;
	}
	printf("card number = %s\n", argv[1]);
	sprintf(card_name, "hw:CARD=%s", argv[1]);

	printf("control name = %s\n", argv[2]);
	ctlname = argv[2];

	/* open the control device for a specific card */
	ret = snd_ctl_open(&ctlp, card_name, SND_CTL_NONBLOCK);
	if(ret < 0) {
		printf("snd_ctl_open failed: %d\n", ret);
		return ret;
	}

	ret = snd_ctl_subscribe_events(ctlp, 1);
	if(ret < 0) {
		printf("snd_ctl_subscribe_events failed: %d\n", ret);
		return ret;
	}

	/* check polling count */
	if (snd_ctl_poll_descriptors_count(ctlp) != 1) {
		printf("snd_ctl_poll_descriptors_count failed\n");
		return -ENXIO;
	}

	/* prepare the list of polling descriptors */
	if (snd_ctl_poll_descriptors(ctlp, &pfds, 1) != 1) {
		printf("snd_ctl_poll_descriptors failed\n");
		return -ENXIO;
	}

	/* start polling */
	ret = poll(&pfds, 1, 10000);
	if (ret < 0)
		return errno;

	if (ret == 0) {
		printf("poll event time out\n");
	}

	/* when poll returned because of an event, get event */
	ret = snd_ctl_poll_descriptors_revents(ctlp, &pfds, ret, &revents);
	if(ret < 0) {
		printf("snd_ctl_poll_descriptors_revents returned %d\n", ret);
		return ret;
	}

	snd_ctl_event_alloca(&ctlevent);

	/* read which event */
	ret = snd_ctl_read(ctlp, ctlevent);
	if (ret < 0) {
		printf("snd_ctl_read returned %d\n", ret);
		return ret;
	}

	event = snd_ctl_event_get_type(ctlevent);
	printf("Event type = %d\n", event);
	if(SND_CTL_EVENT_ELEM != event) {
		printf("wrong event type = %d\n", event);
		return -1;
	}
 
	elename = snd_ctl_event_elem_get_name(ctlevent);
	if(strcmp(elename, ctlname)) {
		printf("not desired control = %s\n", elename);
		return -1;
	}

	/* allocate memory for various buffers */
	snd_ctl_elem_value_alloca(&elemval);
	snd_ctl_elem_id_alloca(&elemid);
	snd_ctl_elem_info_alloca(&eleinfo);

	/* prepare which control element we want to read */
	snd_ctl_elem_id_set_name(elemid, ctlname);
	snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_index(elemid, 0);
	snd_ctl_elem_value_set_id(elemval, elemid);

	/* read the element info */
	ret = snd_ctl_elem_read(ctlp, elemval);
	if (ret < 0) {
		printf("snd_ctl_elem_read failed: %d\n", ret);
		return ret;
	}
	
	/* read how many values to read */
	snd_ctl_elem_info_set_id(eleinfo, elemid);
	ret = snd_ctl_elem_info(ctlp, eleinfo);
	if (ret < 0) {
		printf("snd_ctl_elem_info failed: %d\n", ret);
		return ret;
	}

	count = snd_ctl_elem_info_get_count(eleinfo);
	printf("control count = %d\n", count);

	/* if there is a proper event then read the control value */
	for(i=0; i < count; i++) {
		value = snd_ctl_elem_value_get_integer(elemval, i);
		printf("control value = 0x%x\n", value);
	}

	return ret;

}

