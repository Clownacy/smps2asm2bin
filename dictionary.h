#pragma once

typedef struct DictionaryEntry
{
	struct DictionaryEntry *next;

	char *name;
	long value;
} DictionaryEntry;

extern DictionaryEntry *dictionary_head;
extern const char *undefined_symbol;

void AddDictionaryEntry(const char *name, long value);
long LookupDictionary(const char *name);
void ClearDictionary(void);
