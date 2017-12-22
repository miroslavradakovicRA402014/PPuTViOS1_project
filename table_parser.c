#include "tables.h"

ParseErrorCode parsePatHeader(const uint8_t* patHeaderBuffer, PatHeader* patHeader)
{    
    if(patHeaderBuffer==NULL || patHeader==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    patHeader->tableId = (uint8_t)* patHeaderBuffer; 
    if (patHeader->tableId != 0x00)
    {
        printf("\n%s : ERROR it is not a PAT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;
    
    lower8Bits = (uint8_t)(*(patHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    patHeader->sectionSyntaxIndicator = lower8Bits & 0x01;

    higher8Bits = (uint8_t) (*(patHeaderBuffer + 1));
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 2));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patHeader->sectionLength = all16Bits & 0x0FFF;
    
    higher8Bits = (uint8_t) (*(patHeaderBuffer + 3));
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 4));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patHeader->transportStreamId = all16Bits & 0xFFFF;
    
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    patHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 5));
    patHeader->currentNextIndicator = lower8Bits & 0x01;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 6));
    patHeader->sectionNumber = lower8Bits & 0xFF;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 7));
    patHeader->lastSectionNumber = lower8Bits & 0xFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatServiceInfo(const uint8_t* patServiceInfoBuffer, PatServiceInfo* patServiceInfo)
{
    if(patServiceInfoBuffer==NULL || patServiceInfo==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    higher8Bits = (uint8_t) (*(patServiceInfoBuffer));
    lower8Bits = (uint8_t) (*(patServiceInfoBuffer + 1));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patServiceInfo->programNumber = all16Bits & 0xFFFF; 

    higher8Bits = (uint8_t) (*(patServiceInfoBuffer + 2));
    lower8Bits = (uint8_t) (*(patServiceInfoBuffer + 3));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patServiceInfo->pid = all16Bits & 0x1FFF;
    
    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatTable(const uint8_t* patSectionBuffer, PatTable* patTable)
{
    uint8_t * currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if(patSectionBuffer==NULL || patTable==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    if(parsePatHeader(patSectionBuffer,&(patTable->patHeader))!=TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing PAT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    parsedLength = 12 /*PAT header size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(patSectionBuffer + 8); /* Position after last_section_number */
    patTable->serviceInfoCount = 0; /* Number of services info presented in PAT table */
    
    while(parsedLength < patTable->patHeader.sectionLength)
    {
        if(patTable->serviceInfoCount > TABLES_MAX_NUMBER_OF_PIDS_IN_PAT - 1)
        {
            printf("\n%s : ERROR there is not enough space in PAT structure for Service info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }
        
        if(parsePatServiceInfo(currentBufferPosition, &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 4; /* Size from program_number to pid */
            parsedLength += 4; /* Size from program_number to pid */
            patTable->serviceInfoCount ++;
        }    
    }
    
    return TABLES_PARSE_OK;
}

ParseErrorCode printPatTable(PatTable* patTable)
{
    uint8_t i=0;
    
    if(patTable==NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    printf("\n********************PAT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n",patTable->patHeader.tableId);
    printf("section_length           |      %d\n",patTable->patHeader.sectionLength);
    printf("transport_stream_id      |      %d\n",patTable->patHeader.transportStreamId);
    printf("section_number           |      %d\n",patTable->patHeader.sectionNumber);
    printf("last_section_number      |      %d\n",patTable->patHeader.lastSectionNumber);
    
    for (i=0; i<patTable->serviceInfoCount;i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number           |      %d\n",patTable->patServiceInfoArray[i].programNumber);
        printf("pid                      |      %d\n",patTable->patServiceInfoArray[i].pid); 
    }
    printf("\n********************PAT TABLE SECTION********************\n");
    
    return TABLES_PARSE_OK;
}


ParseErrorCode parsePmtHeader(const uint8_t* pmtHeaderBuffer, PmtTableHeader* pmtHeader)
{

    if(pmtHeaderBuffer==NULL || pmtHeader==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    pmtHeader->tableId = (uint8_t)* pmtHeaderBuffer; 
    if (pmtHeader->tableId != 0x02)
    {
        printf("\n%s : ERROR it is not a PMT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    pmtHeader->sectionSyntaxIndicator = lower8Bits & 0x01;
    
    higher8Bits = (uint8_t) (*(pmtHeaderBuffer + 1));
    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 2));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtHeader->sectionLength = all16Bits & 0x0FFF;

    higher8Bits = (uint8_t) (*(pmtHeaderBuffer + 3));
    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 4));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtHeader->programNumber = all16Bits & 0xFFFF;
    
    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    pmtHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 5));
    pmtHeader->currentNextIndicator = lower8Bits & 0x01;

    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 6));
    pmtHeader->sectionNumber = lower8Bits & 0xFF;

    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 7));
    pmtHeader->lastSectionNumber = lower8Bits & 0xFF;

    higher8Bits = (uint8_t) (*(pmtHeaderBuffer + 8));
    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 9));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtHeader->pcrPid = all16Bits & 0xFFFF;

    higher8Bits = (uint8_t) (*(pmtHeaderBuffer + 10));
    lower8Bits = (uint8_t) (*(pmtHeaderBuffer + 11));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtHeader->programInfoLength = all16Bits & 0x0FFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtElementaryInfo(const uint8_t* pmtElementaryInfoBuffer, PmtElementaryInfo* pmtElementaryInfo)
{
    if(pmtElementaryInfoBuffer==NULL || pmtElementaryInfo==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;
    
	pmtElementaryInfo->streamType = (uint8_t)(*pmtElementaryInfoBuffer);

	higher8Bits = (uint8_t) (*(pmtElementaryInfoBuffer + 1));
    lower8Bits = (uint8_t) (*(pmtElementaryInfoBuffer + 2));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtElementaryInfo->elementaryPid = all16Bits & 0x1FFF;

	higher8Bits = (uint8_t) (*(pmtElementaryInfoBuffer + 3));
    lower8Bits = (uint8_t) (*(pmtElementaryInfoBuffer + 4));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    pmtElementaryInfo->esInfoLength = all16Bits & 0x0FFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtTable(const uint8_t* pmtSectionBuffer, PmtTable* pmtTable)
{
    uint8_t * currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if(pmtSectionBuffer==NULL || pmtTable==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    if(parsePmtHeader(pmtSectionBuffer,&(pmtTable->pmtHeader))!=TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing PMT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    parsedLength = 12 + pmtTable->pmtHeader.programInfoLength /*PMT header size*/ + 4 /*CRC size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(pmtSectionBuffer + 12 + pmtTable->pmtHeader.programInfoLength); /* Position after last descriptor */
    pmtTable->elementaryInfoCount = 0; /* Number of elementary info presented in PMT table */
    
    while(parsedLength < pmtTable->pmtHeader.sectionLength)
    {
        if(pmtTable->elementaryInfoCount > TABLES_MAX_NUMBER_OF_ELEMENTARY_PID - 1)
        {
            printf("\n%s : ERROR there is not enough space in PMT structure for elementary info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }
        
        if(parsePmtElementaryInfo(currentBufferPosition, &(pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 5 + pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount].esInfoLength; /* Size from stream type to elemntary info descriptor*/
            parsedLength += 5 + pmtTable->pmtElementaryInfoArray[pmtTable->elementaryInfoCount].esInfoLength; /* Size from stream type to elementary info descriptor */
            pmtTable->elementaryInfoCount++;
        }    
    }

    return TABLES_PARSE_OK;
}

ParseErrorCode printPmtTable(PmtTable* pmtTable)
{
    uint8_t i=0;
    
    if(pmtTable==NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    printf("\n********************PMT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n",pmtTable->pmtHeader.tableId);
    printf("section_length           |      %d\n",pmtTable->pmtHeader.sectionLength);
    printf("program_number           |      %d\n",pmtTable->pmtHeader.programNumber);
    printf("section_number           |      %d\n",pmtTable->pmtHeader.sectionNumber);
    printf("last_section_number      |      %d\n",pmtTable->pmtHeader.lastSectionNumber);
    printf("program_info_legth       |      %d\n",pmtTable->pmtHeader.programInfoLength);
    
    for (i=0; i<pmtTable->elementaryInfoCount;i++)
    {
        printf("-----------------------------------------\n");
        printf("stream_type              |      %d\n",pmtTable->pmtElementaryInfoArray[i].streamType);
        printf("elementary_pid           |      %d\n",pmtTable->pmtElementaryInfoArray[i].elementaryPid);
    }
    printf("\n********************PMT TABLE SECTION********************\n");
    
    return TABLES_PARSE_OK;
}

ParseErrorCode parseEitHeader(const uint8_t* eitHeaderBuffer, EitTableHeader* eitHeader)
{    
    if(eitHeaderBuffer==NULL || eitHeader==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    eitHeader->tableId = (uint8_t)* eitHeaderBuffer; 
    if (eitHeader->tableId != 0x4E)
    {
        printf("\n%s : ERROR it is not a EIT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;
    
    lower8Bits = (uint8_t)(*(eitHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    eitHeader->sectionSyntaxIndicator = lower8Bits & 0x01;

    higher8Bits = (uint8_t) (*(eitHeaderBuffer + 1));
    lower8Bits = (uint8_t) (*(eitHeaderBuffer + 2));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    eitHeader->sectionLength = all16Bits & 0x0FFF;
    
   	higher8Bits = (uint8_t) (*(eitHeaderBuffer + 3));
   	lower8Bits = (uint8_t) (*(eitHeaderBuffer + 4));
  	all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
	eitHeader->serviceId = all16Bits;    

    lower8Bits = (uint8_t) (*(eitHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    eitHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t) (*(eitHeaderBuffer + 5));
    eitHeader->currentNextIndicator = lower8Bits & 0x01;

    eitHeader->sectionNumber = (uint8_t) (*(eitHeaderBuffer + 6));
	eitHeader->lastSectionNumber = (uint8_t) (*(eitHeaderBuffer + 7));

	higher8Bits = (uint8_t) (*(eitHeaderBuffer + 8));
    lower8Bits = (uint8_t) (*(eitHeaderBuffer + 9));
	all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
	eitHeader->transportStreamId = all16Bits;

	higher8Bits = (uint8_t) (*(eitHeaderBuffer + 10));
    lower8Bits = (uint8_t) (*(eitHeaderBuffer + 11));
	all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
	eitHeader->transportStreamId = all16Bits;

	eitHeader->segmentLastSectionNumber = (uint8_t) (*(eitHeaderBuffer + 12));
	eitHeader->lastTableId = (uint8_t) (*(eitHeaderBuffer + 13));

    return TABLES_PARSE_OK;
}

ParseErrorCode parseEitEventInfo(const uint8_t* eitEventInfoBuffer, EitEventInfo* eitEventInfo)
{
    if(eitEventInfoBuffer==NULL || eitEventInfo==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
	uint8_t i;
	uint16_t offset;
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
	uint8_t descTag = 0;
	uint8_t descLength = 0;
    uint16_t all16Bits = 0;
	uint16_t higher16Bits = 0;
	uint16_t lower16Bits = 0;
	uint32_t all32Bits = 0;


	higher8Bits = (uint8_t) (*eitEventInfoBuffer);
    lower8Bits = (uint8_t) (*(eitEventInfoBuffer + 1));
	all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
	eitEventInfo->eventId = all16Bits;

	higher8Bits = (uint8_t) (*(eitEventInfoBuffer + 2));
    lower8Bits = (uint8_t) (*(eitEventInfoBuffer + 3));
	higher16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);	

	higher8Bits = (uint8_t) (*(eitEventInfoBuffer + 4));
    lower8Bits = (uint8_t) (*(eitEventInfoBuffer + 5));
	lower16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);

	all32Bits = (uint16_t) ((higher16Bits << 16) + lower16Bits);
	eitEventInfo->startTime = all32Bits;

	higher8Bits = (uint8_t) (*(eitEventInfoBuffer + 10));
    lower8Bits = (uint8_t) (*(eitEventInfoBuffer + 11));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);

    eitEventInfo->runningStatus = (uint8_t)((all16Bits & 0xE000) >> 13);
	eitEventInfo->CAmode = (uint8_t)(all16Bits & 0x1000);
	eitEventInfo->descriptorsLoopLength = all16Bits & 0x0FFF;

	offset = 0;

	if (eitEventInfo->runningStatus == 0x04)
	{
		while (offset < eitEventInfo->descriptorsLoopLength)
		{	
		   	descTag = *(eitEventInfoBuffer + 12 + offset);
			descLength = *(eitEventInfoBuffer + 13 + offset);

			if(descTag == 0x4d)
			{
				eitEventInfo->eventNameLength = (uint8_t) (*(eitEventInfoBuffer + 17 + offset));
		        for(i = 0; i < eitEventInfo->eventNameLength; i++)
		        {
		        	eitEventInfo->eventName[i] = (char)(*(eitEventInfoBuffer + 12 + 6 + i + offset));
		        }
		        eitEventInfo->eventName[i] = '\0';

				descLength = *(eitEventInfoBuffer + 18 + eitEventInfo->eventNameLength + offset);

			}		

			printf("Parse more\n %d %d",eitEventInfo->descriptorsLoopLength, offset);

			offset += descLength + 2;
		}
	}

	printf("Parsed passed!\n");

    return TABLES_PARSE_OK;
}

ParseErrorCode parseEitTable(const uint8_t* eitSectionBuffer, EitTable* eitTable)
{
    uint8_t* currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if(eitSectionBuffer==NULL || eitTable==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    if(parseEitHeader(eitSectionBuffer,&(eitTable->eitHeader))!=TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing EIT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    parsedLength = 14 /*EIT header size*/ + 4/*CRC size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(eitSectionBuffer + 14); 
    eitTable->eventInfoCount = 0; /* Number of info presented in EIT table */
    
    while(parsedLength < eitTable->eitHeader.sectionLength)
    {
        if(eitTable->eventInfoCount > TABLES_MAX_NUMBER_OF_EVENTS - 1)
        {
            printf("\n%s : ERROR there is not enough space in EIT structure for elementary info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }
        
        if(parseEitEventInfo(currentBufferPosition, &(eitTable->eitInfoArray[eitTable->eventInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 12 + eitTable->eitInfoArray[eitTable->eventInfoCount].descriptorsLoopLength ; 
           	parsedLength += 12 + eitTable->eitInfoArray[eitTable->eventInfoCount].descriptorsLoopLength;
            eitTable->eventInfoCount++;
        }    
    }

    return TABLES_PARSE_OK;
}

ParseErrorCode printEitTable(EitTable* eitTable)
{
    uint8_t i=0;
    
    if(eitTable==NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    printf("\n********************EIT TABLE SECTION********************\n");
    printf("table_id                 |      %x\n",eitTable->eitHeader.tableId);
    printf("section_syntax_indicator |      %d\n",eitTable->eitHeader.sectionSyntaxIndicator);
    printf("section_length           |      %d\n",eitTable->eitHeader.sectionLength);
    printf("service_id               |      %d\n",eitTable->eitHeader.serviceId);
    printf("versionNumber            |      %d\n",eitTable->eitHeader.versionNumber);
    printf("current_next_indicator   |      %d\n",eitTable->eitHeader.currentNextIndicator);
	printf("section_number    		 |      %d\n",eitTable->eitHeader.sectionNumber);
	printf("last_section_number      |      %d\n",eitTable->eitHeader.lastSectionNumber);
	printf("transport_stream_id      |      %d\n",eitTable->eitHeader.transportStreamId);
	printf("original_network_id      |      %d\n",eitTable->eitHeader.originalNetworkId);
	printf("segment_last_section     |      %d\n",eitTable->eitHeader.segmentLastSectionNumber);
	printf("last_table_id       	 |      %d\n",eitTable->eitHeader.lastTableId);
    
    for (i=0; i<eitTable->eventInfoCount;i++)
    {
        printf("-----------------------------------------\n");
        printf("event_id                |      %d\n",eitTable->eitInfoArray[i].eventId);
		printf("start_time              |      %llx\n",eitTable->eitInfoArray[i].startTime);
		printf("duration                |      %d\n",eitTable->eitInfoArray[i].duration);
		printf("running_status          |      %x\n",eitTable->eitInfoArray[i].runningStatus);
		printf("CA_mode                 |      %ld\n",eitTable->eitInfoArray[i].CAmode);
		printf("descriptors_loop_length |      %ld\n",eitTable->eitInfoArray[i].descriptorsLoopLength);
		printf("event_name_length		|      %ld\n",eitTable->eitInfoArray[i].eventNameLength);
    }
    printf("\n********************EIT TABLE SECTION********************\n");
    
    return TABLES_PARSE_OK;
}




