// Connect to DB
var express= require('express')
var app = express()
var mysql = require('mysql')
var request = require('request');

var connection = mysql.createConnection({
  host : 'localhost',
  user : 'sensor',
  password : 'sensor.data',
  database : 'data', 
  timezone : 'utc'
});

connection.connect();

function isSameArea(light_input, light_db){
  if (light_input >=0 && light_input < 300 && 
      light_db >=0 && light_db < 300) 
    return 1;
  else if (light_input >=300 && light_input < 600 && 
      light_db >=300 && light_db < 600) 
    return 1;
  else if (light_input >=600 && light_input < 1024 && 
      light_db >=600 && light_db < 1024) 
    return 1;
  else 
    return 0;
}


/*********************
 * COMPARE WITH DB   **
 **********************/

app.get("/pattern", function(req, res) {
    console.log("data received");
    console.log("param=" + req.query);

    // give query
    var qstr = 'select * from pattern where HOUR(reg_date) = '
    + req.query.hour;

    connection.query(qstr, function(err, rows, cols){
        if (err) {
          throw err;
          res.send('query error: ' + qstr);

          return;
        }

        var uri
        // get rows and compare
        for (var i=0; i<rows.length; i++){
          if ( isSameArea(req.query.light, rows[i].light) ){
            if ( rows[i].tag_data == -1 ){ // hasn't been touched
              uri = "http://192.168.0.10:8811/pattern";

              var prt = "hour : " + req.query.hour + 
                        " light : " + req.query.light
                       + " pattern : " + rows[i].pattern;
              console.log(prt);
              
              // send request to the NodeMCU
              var pat = rows[i].pattern + 'e';
              var propertiesObject = {pattern: pat};

              request({url:uri, qs:propertiesObject}, 
              function(err, response, body) {
                if(err) { console.log(err); return; }
                console.log("Get response: " + response.statusCode);
              });
              (uri, function(error, response, body){
                console.log(body);
              });
            } else { // user has adjusted before
              uri = "http://192.168.0.145:8811/?pattern=" 
                    + rows[i].pattern + 'e'; 
              // send request to the NodeMCU
              request(uri, function(error, response, body){
                console.log(body);
              });
            }
          }
        }
    }); // end of query
});

app.listen(3000, function(){
    console.log('example app listening on port 3000!')
    });
