var myAPIKey = 'b9958e4005f140ee8e229d9a81b3db65';

var xhrRequest = function(url, type, callback)	{
	var xhr = new XMLHttpRequest();
	xhr.onload = function()	{
		callback(this.responseText);
	};
	xhr.open(type,url);
	xhr.send();
};

//Request weather
function locationSuccess(pos)	{
	//Generate URL	
	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + 
						pos.coords.latitude + '&lon=' + pos.coords.longitude + 
						'&appid=' + myAPIKey;
	
	//Send request to weather service
	xhrRequest(url, 'GET', 
		function(responseText)	{
			//contains JSON object
			var json = JSON.parse(responseText);
			
			//adjust temp from Kelvin
			var temperature = Math.round((json.main.temp - 273.15)*(9/5)+32);
			console.log('Temperature is ' + temperature);
			
			//conditions
			var conditions = json.weather[0].main;
			console.log('Conditions are ' + conditions);
		
			//Assemble dictionary using keys
			var dictionary = {
				'TEMPERATURE':temperature,
				'CONDITIONS':conditions
			};
			
			//Send to Pebble
			Pebble.sendAppMessage(dictionary,
				function(e)	{
					console.log('Weather info sent successfully');
				},
				function(e)	{
					console.log('Error sending weather info');
				});
			
			//Receive message
			Pebble.addEventListener('appmessage',
				function(e)	{
					console.log('AppMessage received');
					getWeather();
				});			
		});
}

function locationError(err)	{
	console.log('Error requestion location');
}

function getWeather()	{
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000});
}

//Listen for opening of watchface
Pebble.addEventListener('ready',
	function(e)	{
		console.log('PebbleKit JS ready!');
		
		//Get initial weather
		getWeather();
	});

//Listen for recived AppMessage
Pebble.addEventListener('appmessage',
	function(e)	{
		console.log('AppMessage received!');
	});

