/*
 * weather.c
 *
 * Created on: May 14, 2020
 *		Author: yfs
 *
 * Implements the weather thread
 */
 
#include "main.h"

/*
Defines for the weather thread
*/
#define JSON_START											('{')

/* Buffers */
#define MAX_HTTP_RECEIVE_CHARS								(3000)
#define MAX_HTTP_SEND_CHARS									(300)


static void wifi_init( void );
static void display_update( char* http_data, int num_bytes );

/* wifi_init()
 *
 * Turn on the Wi-Fi and connect to the access point
 */
void wifi_init( void )
{
	cy_rslt_t result;

	/* Variables used by Wi-Fi connection manager */
	cy_wcm_config_t wifi_config;
	cy_wcm_connect_params_t wifi_conn_param;
	cy_wcm_ip_address_t ip_address;

	/*
	Initialize the Wi-Fi connection manager (WCM)
	*/
	wifi_config.interface = CY_WCM_INTERFACE_TYPE_STA;
	cy_wcm_init( &wifi_config );

	/*
	Connect to the Access Point (AP)
		Note: AP_NAME and SSID are #define strings in private_data.h
		e.g. #define AP_NAME "MY_SSID"
	*/
	 memset( &wifi_conn_param, 0, sizeof( cy_wcm_connect_params_t ) );
	 memcpy( wifi_conn_param.ap_credentials.SSID, AP_NAME, strlen( AP_NAME ) );
	 memcpy( wifi_conn_param.ap_credentials.password, PASSWORD, strlen( PASSWORD ) );
	 wifi_conn_param.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;

	printf( "Connecting to access point\n" );
	for( ; ; )
	{
		result = cy_wcm_connect_ap( &wifi_conn_param, &ip_address );
		if( CY_RSLT_SUCCESS == result )
			{ break; }
		printf( "Retrying...\n" );
	}
	
	PRINT_IPV4_ADDRESS( "Local IP", ip_address.ip.v4 );
}

/* weather_task()
 *
 * Turn on Wi-Fi
 * Connect to access point
 * Send HTTP requet to weatherstack.com
 * Receive and process returned JSON
 */  
void weather_task( void* arg )
{
	cy_rslt_t result;
	
	char* city;

	uint32_t bytes_sent = 0;
	uint32_t bytes_recd = 0;
	
	static char send_buf[MAX_HTTP_SEND_CHARS];				// Non-stack arrays with local scope
	static char recv_buf[MAX_HTTP_RECEIVE_CHARS];

	wifi_init();

	/* Initialize secure socket library */
	result = cy_socket_init();
	CY_ASSERT( result == CY_RSLT_SUCCESS );

	/*
	Set up the socket variables to connect to the weatherstack IP address
	*/
	cy_socket_t socket_handle;								// Handle for all socket transactions
	cy_socket_sockaddr_t socket_addr =
	{
		.port = HTTP_PORT_NUMBER,
		.ip_address = 
		{
			.version = CY_SOCKET_IP_VER_V4,
			.ip = { .v4 = 0 /* Set with gethostbyname() */ }
		}
	};
	
	result = cy_socket_gethostbyname( "api.weatherstack.com", CY_SOCKET_IP_VER_V4, &( socket_addr.ip_address ) );
	CY_ASSERT( result == CY_RSLT_SUCCESS );
	PRINT_IPV4_ADDRESS( "Remote IP", socket_addr.ip_address.ip.v4 );

/*
	Task loop waits for a city, sends the request, receives the response and displays the data
	*/
	for( ; ; )
	{
		/*
		Sleep until user requests data from the server by sending a pointer to the city name
		*/
		printf( "Press CapSense button to send/receive\n" );
		city = (char*) ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		/*
		Create the HTTP message
		*/
		sprintf( send_buf, "GET /current?access_key=%s&query=%s HTTP/1.1\r\n", ACCESS_KEY, city );
		strcat( send_buf, "Host: api.weatherstack.com\r\n\r\n" );
		printf( "Sending request for weather in %s\n", city );

		/*
		Make the weather API transaction - socket create / send / receive / disconnect / delete
		*/
		result = cy_socket_create( CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM, CY_SOCKET_IPPROTO_TCP, &socket_handle );
		CY_ASSERT( result == CY_RSLT_SUCCESS );

		result = cy_socket_connect( socket_handle, &socket_addr, sizeof( socket_addr ) );
		CY_ASSERT( result == CY_RSLT_SUCCESS );

		result = cy_socket_send( socket_handle, send_buf, strlen( send_buf ), 0, &bytes_sent );
		CY_ASSERT( result == CY_RSLT_SUCCESS );
		printf( "%d bytes sent\n", (int)bytes_sent );

		result = cy_socket_recv( socket_handle, recv_buf, MAX_HTTP_RECEIVE_CHARS, 0, &bytes_recd );
		printf( "%d bytes received (%s)\n", (int)bytes_recd, ( result == CY_RSLT_SUCCESS ) ? "success" : "failure" );

		result = cy_socket_disconnect( socket_handle, 0 );
		CY_ASSERT( result == CY_RSLT_SUCCESS );
		
		result = cy_socket_delete( socket_handle );
		CY_ASSERT( result == CY_RSLT_SUCCESS );

		display_update( recv_buf, bytes_recd );
	}
}

/* display_update()
 *
 * Extract information from JSON and display on UART and TFT (if supported)
 */
static void display_update( char* http_data, int num_bytes )
{
	/*
	Extract the current conditions from the JSON returned from weatherstack. Data format:

	weather_data ->	request
					location
					current ->	weather_descriptions[]
								wind_dir
								precip
	*/
	CY_ASSERT( http_data );

	cJSON* weather_data;									// Root of the JSON data */
	cJSON* current;											// Pointer to the current weather */
	cJSON* item;											// Pointer to the actual data */

	/* New font style and color in this thread */
	#ifdef TFT_SUPPORTED
	GUI_SetColor( GUI_DARKGREEN );							// Low contrast for laptop camera
	GUI_SetFont( &GUI_Font32_ASCII );						// Big, clear font
	#endif
	
	/*
	Scan the HTTP message for JSON data
	*/
	while( JSON_START != *http_data )
	{
		CY_ASSERT( num_bytes > 0 );
		num_bytes--;
		http_data++;
	}

	/* Parse the JSON so we know it is good */
	weather_data = cJSON_Parse( http_data );
	CY_ASSERT( NULL != weather_data );

	/* Extract pointer to the current weather */
	current  = cJSON_GetObjectItemCaseSensitive( weather_data, "current" );

	/*
	Extract the current weather from array of conditions: "weather_descriptions" : [ "cloudy" ]
	*/
	item = cJSON_GetObjectItemCaseSensitive( current, "weather_descriptions" );
	item = cJSON_GetArrayItem( item, 0 );					// Use the zeroth element of the array
	CY_ASSERT( cJSON_IsString( item ) );

	printf( "Conditions: %s\n", item->valuestring );
	#ifdef TFT_SUPPORTED
	GUI_DispStringAt( item->valuestring, TFT_LEFT_ALIGNED, TFT_ROW_WEATHER );
	#endif

	/*
	Extract the wind direction (wind_dir)
	*/
	item = cJSON_GetObjectItemCaseSensitive( current, "wind_dir" );
	CY_ASSERT( cJSON_IsString( item ) );
	
	printf( "Wind:\t\t%s\n", item->valuestring );
	#ifdef TFT_SUPPORTED
	GUI_DispStringAt( "Wind:  ", TFT_LEFT_ALIGNED, TFT_ROW_WIND );
	GUI_DispString( item->valuestring );
	#endif

	/*
	Extract the rain (precip)
	*/
	item = cJSON_GetObjectItemCaseSensitive( current, "precip" );
	CY_ASSERT( cJSON_IsNumber( item ) );

	printf( "Rain:\t\t%d %%\n", item->valueint );	
	#ifdef TFT_SUPPORTED
	GUI_DispStringAt( "Rain:   ", TFT_LEFT_ALIGNED, TFT_ROW_RAIN );
	GUI_DispDecMin( item->valueint );
	GUI_DispString( " %" );
	#endif

	/*
	Extract the temperature
	*/
	item = cJSON_GetObjectItemCaseSensitive( current, "temperature" );
	CY_ASSERT( cJSON_IsNumber( item ) );

	printf( "Temp:\t\t%d C\n", item->valueint );
	#ifdef TFT_SUPPORTED
	GUI_DispStringAt( "Temp: ", TFT_LEFT_ALIGNED, TFT_ROW_TEMP );
	GUI_DispDecMin( item->valueint );
	GUI_DispString( " C" );
	#endif

	cJSON_Delete( weather_data );
}
