
#include "PiOMXUser.h"
#include "PiSignageLogging.h"

extern "C"
{
#include "bcm_host.h"
#include "ilclient/ilclient.h"
}

PiOMXUser::PiOMXUser()
{
    if ((client = ilclient_init()) == NULL) {
    	pis_logMessage(PIS_LOGLEVEL_ERROR,"PiOMXUser: Failed to init ilclient\n");
		return;
    }else{
    	pis_logMessage(PIS_LOGLEVEL_ERROR,"PiOMXUser: ilclient loaded.\n");
    }
    if (OMX_Init() != OMX_ErrorNone) {
	    ilclient_destroy(client);
		pis_logMessage(PIS_LOGLEVEL_ERROR,"PiOMXUser: Failed to init OMX\n");
		return;
    }
}

PiOMXUser::~PiOMXUser()
{
    pis_logMessage(PIS_LOGLEVEL_ERROR,"PiOMXUser: DESTROYED\n");
    if(OMX_Deinit() != OMX_ErrorNone)
    	pis_logMessage(PIS_LOGLEVEL_ERROR,"PiOMXUser: Component did not Deinit()\n");

    if (client != NULL) {
    	ilclient_destroy(client);
    }
}