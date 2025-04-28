var createError = require('http-errors'); // imports http errors module,provides tools/functions creates specific HTTP status codes
var express = require('express');  // imports express module, a web framework for node.js. provides routing, middleware and other tools
var path = require('path');  // imports path module from node.js, sue dfor handling and transforming file paths.Useful for work with directories and files in a platform independent way
var cookieParser = require('cookie-parser'); // imports the cookie parser middleware express. used toparsse cookies attached to incoming requests.
var logger = require('morgan'); //imports the morgan logging middleware for express. auto logs HTTP requests to console,helps debugging and monitoring 
var favicon = require('serve-favicon'); // imports the serve favicon middleware. serves the favicon(small icon displayed in browser tab) for the website.
var sensorsRouter = require('./routes/sensors'); // Imports sensors route (sensors.js)

//var indexRouter = require('./routes/index');// imports index router defined in the routes/index.js file.router handles requests for main or home route of the application
var hbs = require('express-handlebars'); // imports express handlebars module, templating engine used with Express.Allows dynmaically generate HTML views injecting data to predefined templates(views)

var app = express();// creates an instance of the express application. instance used to define routes,middleware and configure app to handle incoming requests


// view engine setup
app.engine('hbs', hbs({extname: 'hbs', defaultLayout: 'layout', layoutsDir: __dirname + '/views/layouts'})); // sets up handlebars as template engine for Express app. use files with .hbs extension.specifies default layout to be layout.hbs
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'hbs');

// uncomment after placing your favicon in /public
//app.use(favicon(path.join(__dirname, 'public', 'favicon.ico')));

app.use(logger('dev'));

app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/api', sensorsRouter); // maps sensor data route under /api

//app.use('/', indexRouter);

// catch 404 and forward to error handler
app.use(function(req, res, next) {
  next(createError(404));
});

// error handler
app.use(function(err, req, res, next) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

module.exports = app;
