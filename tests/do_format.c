#include <locale.h>
#include "newsraft.h"

int
main(void)
{
	setlocale(LC_ALL, "");
	struct wstring *w = wcrtes(200);
	struct format_arg args[] = {
		{L'n',  L'd',  {.i = 567                           }},
		{L'i',  L's',  {.s = "The Box"                     }},
		{L'l',  L's',  {.s = "Se7en"                       }},
		{L't',  L's',  {.s = "Cars"                        }},
		{L'o',  L's',  {.s = NULL                          }},
		{L'O',  L's',  {.s = "The Truman Show"             }},
		{L'b',  L's',  {.s = "Shrek"                       }},
		{L'f',  L'd',  {.i = 38395629                      }},
		{L'e',  L's',  {.s = "Алёша Попович и Тугарин Змей"}},
		{L'd',  L's',  {.s = "The Usual Suspects"          }},
		{L'h',  L's',  {.s = "英雄"                        }},
		{L'u',  L's',  {.s = NULL                          }},
		{L'a',  L's',  {.s = "Scarface"                    }},
		{L'm',  L's',  {.s = "Остров сокровищ"             }},
		{L'q',  L's',  {.s = "Big"                         }},
		{L'p',  L's',  {.s = "The Green Mile"              }},
		{L's',  L's',  {.s = "伍六七"                      }},
		{L'g',  L's',  {.s = "Ворошиловский стрелок"       }},
		{L'j',  L's',  {.s = "The Shawshank Redemption"    }},
		{L'y',  L's',  {.s = NULL                          }},
		{L'k',  L's',  {.s = "Knockin' on Heaven's Door"   }},
		{L'w',  L's',  {.s = "Брат"                        }},
		{L'v',  L's',  {.s = "Wag the Dog"                 }},
		{L'\0', L'\0', {.i = 0                             }},
	};

	do_format(w, L"%n%l%f", args);
	if (wcscmp(w->ptr, L"567Se7en38395629") != 0) goto error;

	do_format(w, L"%v%%%p", args);
	if (wcscmp(w->ptr, L"Wag the Dog%The Green Mile") != 0) goto error;

	do_format(w, L"%%%d%%", args);
	if (wcscmp(w->ptr, L"%The Usual Suspects%") != 0) goto error;

	do_format(w, L"%m%", args);
	if (wcscmp(w->ptr, L"Остров сокровищ") != 0) goto error;

	do_format(w, L"%%%%%%%%%", args);
	if (wcscmp(w->ptr, L"%%%%") != 0) goto error;

	do_format(w, L" %q %w ", args);
	if (wcscmp(w->ptr, L" Big Брат ") != 0) goto error;

	do_format(w, L" %q%%w ", args);
	if (wcscmp(w->ptr, L" Big%w ") != 0) goto error;

	do_format(w, L"%s %h %n", args);
	if (wcscmp(w->ptr, L"伍六七 英雄 567") != 0) goto error;

	do_format(w, L"%bгыгы%f", args);
	if (wcscmp(w->ptr, L"Shrekгыгы38395629") != 0) goto error;

	do_format(w, L"гугу%t%w", args);
	if (wcscmp(w->ptr, L"гугуCarsБрат") != 0) goto error;

	do_format(w, L"%a%qгого", args);
	if (wcscmp(w->ptr, L"ScarfaceBigгого") != 0) goto error;

	free_wstring(w);
	return 0;
error:
	free_wstring(w);
	return 1;
}
