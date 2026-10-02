/*
 * Serial console program to scan for WiFi APs, for a board with no LCD.
 * Selected by CONFIG_APP_MAIN_CONSOLE_SCAN; main.cpp is the LVGL program.
 *
 * The main thread reads the console and prints what arrives in its own MessageQueue.
 * A WifiScanner thread does the scanning, and nothing else touches WiFi.
 *
 * This file uses stdio for the console, which strpp has no primitives for yet.
 */
#include "sdkconfig.h"

#if defined(CONFIG_APP_MAIN_CONSOLE_SCAN)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "thread.h"
#include "msgqueue.h"
#include "window.h"
#include "wifi_scanner.h"

extern "C" {
	void app_main();
}

static const char*	PROMPT = "wifi> ";

static const char*	HELP =
	"scan          scan now\n"
	"auto <secs>   scan every <secs> seconds; 'auto 0' stops\n"
	"info          free memory and stack use\n"
	"tasks         list every FreeRTOS task\n"
	"quit          stop the scanner thread\n"
	"help          this text\n";

// The USB console sends its output to the host only on a newline, or when told to
static void
flush()
{
	fflush(stdout);
	fsync(fileno(stdout));
}

class	Console
{
public:
	Console(WifiScanner& a_scanner, MessageQueue& a_inbox)
	: scanner(a_scanner)
	, inbox(a_inbox)
	, length(0)
	, after_cr(false)
	, stopping(false)
	, stopped(false)
	{}

	void		loop();

private:
	WifiScanner&	scanner;
	MessageQueue&		inbox;
	char		line[80];
	size_t		length;
	bool		after_cr;
	bool		stopping;	// A quit request is sent
	bool		stopped;	// The scanner has ended

	void		prompt();
	void		key(int c);
	void		command();
	void		request(const char* name, int argument = -1);
	void		show(const Variant& message);
	void		show_scan();
	void		info();
	void		tasks();
};

void
Console::prompt()
{
	printf("%s%.*s", PROMPT, (int)length, line);
	flush();
}

void
Console::request(const char* name, int argument)
{
	if (stopped || stopping)
	{
		printf("The scanner has stopped; reset the board to restart it\n");
		return;
	}
	VariantArray	message = VariantArray() << name;
	if (argument >= 0)
		message << argument;
	scanner.requests.push(Variant(message));
}

void
Console::info()
{
	printf("Heap free %u, lowest ever %u\n",
		(unsigned)esp_get_free_heap_size(), (unsigned)esp_get_minimum_free_heap_size());
	printf("Stack bytes never used: console %u, scanner %u\n",
		(unsigned)uxTaskGetStackHighWaterMark(NULL),
		(unsigned)(stopped ? 0 : uxTaskGetStackHighWaterMark(scanner.id())));

	Window<WifiScanner>	w(scanner, Milliseconds(100));
	if (w.holding())
		printf("Scanner: %s, %u scans, auto %ld ms\n", w->ready ? "running" : "not running", w->scan_count, w->auto_ms);
	else
		printf("Scanner: busy\n");
}

void
Console::tasks()
{
	size_t		size = 64 * (size_t)uxTaskGetNumberOfTasks();
	char*		text = (char*)malloc(size);
	if (!text)
	{
		printf("Out of memory listing tasks\n");
		return;
	}
	vTaskList(text);
	printf("Name            State  Prio  Stack-free  Num  Core\n%s", text);
	free(text);
}

void
Console::command()
{
	line[length] = '\0';
	length = 0;

	char*		word = strtok(line, " \t");
	if (!word)
		return;
	char*		arg = strtok(NULL, " \t");

	if (!strcmp(word, "scan"))
		request("scan");
	else if (!strcmp(word, "auto"))
	{
		char*		end = 0;
		long		secs = arg ? strtol(arg, &end, 10) : -1;
		if (!arg || *end || secs < 0 || secs > 86400)
			printf("auto needs a number of seconds, from 0 to 86400\n");
		else
			request("auto", (int)(secs*1000));
	}
	else if (!strcmp(word, "info"))
		info();
	else if (!strcmp(word, "tasks"))
		tasks();
	else if (!strcmp(word, "quit"))
	{
		request("quit");
		stopping = true;
	}
	else if (!strcmp(word, "help"))
		printf("%s", HELP);
	else
		printf("Unknown command '%s'. Try 'help'\n", word);
}

void
Console::key(int c)
{
	bool		was_cr = after_cr;
	after_cr = false;

	if (c == '\r' || c == '\n')
	{
		if (c == '\n' && was_cr)
			return;			// The second half of CR LF
		after_cr = (c == '\r');
		printf("\n");
		command();
		prompt();
	}
	else if (c == 0x7f || c == 0x08)
	{
		if (length > 0)
		{
			length--;
			printf("\b \b");
			flush();
		}
	}
	else if (c >= 0x20 && c < 0x7f && length < sizeof line-1)
	{
		line[length++] = (char)c;
		putchar(c);
		flush();
	}
}

void
Console::show_scan()
{
	VariantArray	aps;
	{
		Window<WifiScanner>	w(scanner, Milliseconds(1000));
		if (!w.holding())
		{
			printf("The scanner is busy; the access points are not available yet\n");
			return;
		}
		aps = w->last_scan;
	}
	printf("%u access points\n", (unsigned)aps.length());
	if (aps.length() == 0)
		return;
	printf(" %-32s %4s %3s %-9s %s\n", "SSID", "dBm", "Ch", "Auth", "BSSID");
	for (VariantArray::Index i = 0; i < aps.length(); i++)
	{
		VariantArray	ap = aps[i].as_variant_array();
		StrVal		ssid = ap[0].as_strval();
		StrVal		auth = ap[3].as_strval();
		StrVal		bssid = ap[4].as_strval();
		printf(" %-32s %4d %3d %-9s %s\n",
			ssid.asUTF8(), ap[1].as_int(), ap[2].as_int(),
			auth.asUTF8(), bssid.asUTF8());
	}
}

void
Console::show(const Variant& message)
{
	printf("\r\033[K");		// Clear the prompt and what was typed after it
	if (message.type() != Variant::VarArray || message.as_variant_array().length() == 0)
		printf("Unrecognised message from the scanner\n");
	else
	{
		VariantArray	parts = message.as_variant_array();
		StrVal		name = parts[0].as_strval();
		if (name == "ready")
			printf("WiFi started. Type 'help' for commands\n");
		else if (name == "scan")
			show_scan();
		else if (name == "error")
			printf("Error: %s\n", parts[1].as_strval().asUTF8());
		else if (name == "quit")
		{
			scanner.join();
			stopped = true;
			printf("Scanner stopped\n");
		}
		else
			printf("Unrecognised message '%s'\n", name.asUTF8());
	}
	prompt();
}

void
Console::loop()
{
	prompt();
	for (;;)
	{
		bool		idle = true;

		Variant		message;
		while (inbox.try_pop(message))
		{
			show(message);
			idle = false;
		}

		int		c = getchar();
		if (c == EOF)
			clearerr(stdin);	// Nothing typed: the console doesn't block
		else
		{
			key(c);
			idle = false;
		}

		if (idle)
			Thread::yield(Milliseconds(20));
	}
}

void
app_main()
{
	MainThread	main_thread;

	setvbuf(stdin, NULL, _IONBF, 0);

	MessageQueue*		inbox = MessageQueue::mine();
	WifiScanner	scanner(*inbox);
	Console		console(scanner, *inbox);
	console.loop();
}

#endif	// CONFIG_APP_MAIN_CONSOLE_SCAN
