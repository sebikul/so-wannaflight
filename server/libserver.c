#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "libserver.h"


DB_DATAGRAM* execute_datagram(DB_DATAGRAM* datagram) {

	DB_DATAGRAM* ans;

	switch (datagram->opcode) {

	case OP_CONSULT:
		ans = db_consult_flights(datagram->dg_origin, datagram->dg_destination);
		break;

	case OP_PURCHASE:

		ans = malloc(sizeof(DB_DATAGRAM));
		ans->size = sizeof(DB_DATAGRAM);
		ans->opcode = OP_PURCHASE;
		ans->dg_seat = db_purchase(datagram->dg_flightid);
		break;

	case OP_CANCEL:

		ans = malloc(sizeof(DB_DATAGRAM));
		ans->size = sizeof(DB_DATAGRAM);
		ans->opcode = OP_CANCEL;

		ans->dg_result = db_cancel(datagram->dg_seat);
		break;

	case OP_PING: {

		int size = sizeof(DB_DATAGRAM) + strlen(datagram->dg_cmd) + 1;

		ans = malloc(size);
		ans->size = size;
		ans->opcode = OP_PONG;
		strcpy(ans->dg_cmd, datagram->dg_cmd);
		break;
	}

	case OP_ADDFLIGHT: {

		DB_ENTRY entry = datagram->dg_results[0];

		ans = malloc(sizeof(DB_DATAGRAM));
		ans->size = sizeof(DB_DATAGRAM);
		ans->opcode = OP_ADDFLIGHT;

		ans->dg_flightid = db_add_flight(entry.departure, entry.origin, entry.destination);

		break;
	}

	default:
		return NULL;
	}

#ifdef MSGQUEUE
	ans->sender = datagram->sender;
#endif

	return ans;

}
