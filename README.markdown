# ngx_header_microtime
## Add a microsecond timestamp header.

*This module is not distributed with the Nginx source.*

# Description

This module simply populates a header with the current microsecond epoch from the
server.

# Configuration

	location = /.well-known/time {
		# enable header for a location
		header_microtime on;
		# header_microtime_name "X-HTTPSTIME"; # this is the default but it can be changed
		return 204;
	}
