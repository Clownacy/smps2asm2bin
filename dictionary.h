#pragma once

typedef struct DictionaryEntry
{
	struct DictionaryEntry *next;

	char *name;
	unsigned int value;
} DictionaryEntry;

extern DictionaryEntry *dictionary_head;
extern const char *undefined_symbol;

void AddDictionaryEntry(const char *name, unsigned int value);
unsigned int LookupDictionary(const char *name);
void ClearDictionary(void);
