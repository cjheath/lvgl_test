#if !defined(WIFI_SCANNER_H)
#define WIFI_SCANNER_H
/*
 * WifiScanner: a Thread that owns the WiFi driver and scans for access points.
 * See ReadMe.md for the messages it accepts and sends.
 */
#include	<esp_err.h>
#include	<thread.h>
#include	<msgqueue.h>

class	WifiScanner
: public Thread
{
public:
	// Starts the thread; it sends every reply to "replies", which must outlive it
	WifiScanner(MessageQueue& replies);

	int		run();

	MessageQueue		requests;	// Push a Variant(VariantArray) here, see ReadMe.md

private:
	MessageQueue&		replies;
	long		auto_ms;	// Scan again after this long with no request; 0 = never

	bool		start_wifi();
	bool		check(esp_err_t err, const char* what);
	void		scan();
	void		reply(const VariantArray& message);
	void		reply_error(const char* what, const char* detail);
};

#endif	// WIFI_SCANNER_H
