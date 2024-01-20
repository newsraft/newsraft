#include <locale.h>
#include "newsraft.h"

static bool
test_fmt(struct wstring *out, const wchar_t *fmt, struct format_arg *args, const wchar_t *expect)
{
	do_format(out, fmt, args);
	if (wcscmp(out->ptr, expect) != 0) {
		fprintf(stderr, "%ls != %ls\n", out->ptr, expect);
		return false;
	}
	return true;
}

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

	if (!test_fmt(w, L"%n%l%f",    args, L"567Se7en38395629"))           goto error;
	if (!test_fmt(w, L"%v%%%p",    args, L"Wag the Dog%The Green Mile")) goto error;
	if (!test_fmt(w, L"%%%d%%",    args, L"%The Usual Suspects%"))       goto error;
	if (!test_fmt(w, L"%m%",       args, L"Остров сокровищ"))            goto error;
	if (!test_fmt(w, L"%%%%%%%%%", args, L"%%%%"))                       goto error;
	if (!test_fmt(w, L" %q %w ",   args, L" Big Брат "))                 goto error;
	if (!test_fmt(w, L" %q%%w ",   args, L" Big%w "))                    goto error;
	if (!test_fmt(w, L"%s %h %n",  args, L"伍六七 英雄 567"))            goto error;
	if (!test_fmt(w, L"%bгыгы%f",  args, L"Shrekгыгы38395629"))          goto error;
	if (!test_fmt(w, L"гугу%t%w",  args, L"гугуCarsБрат"))               goto error;
	if (!test_fmt(w, L"%a%qгого",  args, L"ScarfaceBigгого"))            goto error;
	if (!test_fmt(w, L"%13e",      args, L"Алёша Попович"))              goto error;
	if (!test_fmt(w, L"%-13e",     args, L"Алёша Попович"))              goto error;
	if (!test_fmt(w, L"%10s",      args, L"    伍六七"))                 goto error;
	if (!test_fmt(w, L"%-10s",     args, L"伍六七    "))                 goto error;
	if (!test_fmt(w, L"%3h",       args, L" 英"))                        goto error;
	if (!test_fmt(w, L"%-3h",      args, L"英 "))                        goto error;

	free_wstring(w);
	return 0;
error:
	free_wstring(w);
	return 1;
}
