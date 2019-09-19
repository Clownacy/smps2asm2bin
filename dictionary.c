#include "dictionary.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

DictionaryEntry *dictionary_head;

const char *undefined_symbol;

void AddDictionaryEntry(const char *name, unsigned int value)
{
	for (DictionaryEntry *entry = dictionary_head; entry != NULL; entry = entry->next)
		if (strcmp(entry->name, name) == 0)
			PrintError("Error: Symbol '%s' double-defined\n");

	DictionaryEntry *entry = malloc(sizeof(*entry));
	entry->next = dictionary_head;
	dictionary_head = entry;

	entry->name = malloc(strlen(name) + 1);
	strcpy(entry->name, name);
	entry->value = value;
}

unsigned int LookupDictionary(const char *name)
{
	// Check if the symbol is actually a literal
	if (name[0] == '-' || name[0] == '$' || (name[0] >= '0' && name[0] <= '9'))
	{
		const bool negative = name[0] == '-';

		if (negative)
			++name;

		unsigned int value;

		if (name[0] == '$')
		{
			// Hexadecimal literal
			value = strtol(name + 1, NULL, 0x10);
		}
		else
		{
			// Decimal literal
			value = strtol(name, NULL, 10);
		}

		if (negative)
			value = -value;

		return value;
	}

	// Failing that, look up the symbol in the dictionary
	for (DictionaryEntry *entry = dictionary_head; entry != NULL; entry = entry->next)
		if (strcmp(entry->name, name) == 0)
			return entry->value;

	undefined_symbol = name;
	return 0;
}

void ClearDictionary(void)
{
	DictionaryEntry *entry = dictionary_head;
	while (entry != NULL)
	{
		DictionaryEntry *next_entry = entry->next;
		free(entry);
		entry = next_entry;
	}

	dictionary_head = NULL;
}
