% Here we open the EVT-2 device:
devs = evt2('list');
listDevs(devs);

x = input('Enter a number: ');

handle = evt2('open', x);

evt2('clear', handle); % clear outputs
evt2('pulse', handle, 170, 1000); % First number is value, 2nd is duration in ms.

evt2('close', handle);