#ifndef MAIN_H
#define MAIN_H
typedef struct _PROCESS_LIST_ENTRY
{
	LIST_ENTRY Entry;
	WCHAR NameBuffer[MAX_PATH];

}PROCESS_LIST_ENTRY, *PPROCESS_LIST_ENTRY;
#endif