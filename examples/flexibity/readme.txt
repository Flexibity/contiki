Flexibity sensors interface uses JSON format, with HTTP server on port 80 (if
configured with pull=y) or by pushing data objects to push_url with push_freq
frequency. When push variant is used, a data object with embedded desc object
is transferred as a POST request.

All together, the sensor interface is provided by the following access points:

/cfg?pull=y/n;push_url=url;push_freq=sec
{
  'pull': 'y/n',
  'push_url': 'url',
  'push_freq': sec
}

/desc
{
  'type': 'string',
  'version': number,
  'data': [
    { 'name': 'unit' },
     ...
  ]
  'action': [
    'name', ...
  ]
}

/data
{
  'desc': (optional for push calls only),
  'data': [
    { 'name': value },
    ...
  ]
}

/action?name=value
OK/FAIL

