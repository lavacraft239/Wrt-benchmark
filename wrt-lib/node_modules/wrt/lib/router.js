"use scrict";

var React = require('react/addons');
var xurl = require('xurl');
var utils = require('./utils');

const PATH = 1;
const QUERY = 2;
const HASH_PATH = 3;
const HASH_QUERY = 4;

var Router = module.exports = React.createClass({
  getInitialState: function() {
    return {
      data: this.props.data,
      root: this.normalizePath(this.props.root),
      route: null,
      oldRoute: null,
      routes: {},
      location: this.props.location || window.location,
    };
  },
  getContext: function(fragment) {
    var context;
    switch(fragment[0]) {
      case '/': context = PATH;  break;
      case '?': context = QUERY; break;
      case '#': context = fragment[1] === '?' ? HASH_QUERY : HASH_PATH; break;
      default: throw new Error("Can't get url context");
    }
    return context;
  },
  getLocationKey: function(fragment) {
    var locationKey;
    if (fragment[0] === '#' && fragment[1] === '?') {
      locationKey = fragment.substr(2);
    } else {
      locationKey = fragment.substr(1);
    }
    return locationKey;
  },
  normalizePath: function(path) {
    return '/' + (path || '').replace(/^\//, '');
  },
  linkRoute: function(normalizedPath, context) {
    context = this.state.context || context;
    if (context === HASH_PATH) {
      return '#' + normalizedPath;
    } else {
      console.info('not implemented yet');
    }
  },
  getRouteFromLocation: function(locationHref) {
    var routes = this.state.routes;
    for (var route in routes) {
      if (Object.hasOwnProperty.call(routes, route)) {
        var routeObj = this.state.routes[route];
        if (routeObj.regex.test(locationHref)) {
          return routeObj.Route;
        }
      }
    }
    return this.state.defaultRoute;
  },
  setupHashchange: function() {
    if ("onhashchange" in window) {
      window.addEventListener('hashchange', this.onHashchange);
    } else {
      console.info("browser doesn't support hashchange");
    }
  },
  onHashchange: function() {
    this.forceUpdate();
  },
  globToRegex: function(glob) {
    var namedGoups = [];
    var regex = RegExp('^' + glob.replace(/\//g, '\\/').replace(/:(\w+)/g, function(all, namedGoup) {
        namedGoups.push(namedGoup);
        return '([^\\/]+)';
    }) + '$');
    return {regex, namedGoups};
  },
  componentWillMount: function() {
    var routes = {};
    var context = this.getContext(this.props.root || '/');
    var locationKey = this.getLocationKey(this.props.root || '/');
    var defaultRoute;
    React.Children.forEach(this.props.children, function(child) {
      var normalizedPath = this.normalizePath(child.props.path);
      var {regex, namedGoups} = this.globToRegex(normalizedPath);

      var route = {
        href: this.linkRoute(normalizedPath, context),
        regex: regex,
        namedGoups: namedGoups,
        Route: child,
      };
      routes[child.props.name || normalizedPath.substr(1) || '/'] = route;
      if (child.props.default) {
        defaultRoute = child;
      }
    }, this);

    var updatedState = {
      routes: routes,
      context: context,
      locationKey: locationKey,
    };
    if (defaultRoute) {
      updatedState.defaultRoute = defaultRoute;
    }
    if (context === QUERY || context == HASH_QUERY) {
      updatedState.pathRegex = RegExp('((?:^|[&?])' + locationKey + '=)([^&]*)');
    }
    this.setState(updatedState);

    if (typeof location === 'object' && location === this.state.locaton) {
      this.setupHashchange();
    }
    // if (typeof location === 'object' && location === this.state.locaton) {
    //   this.setupPopState();
    // }
  },
  componentDidMount: function() {
    console.log('mounted Router');
  },
  componentWillUnmount: function() {
    console.log('unmounting Router');
  },
  renderRoute: function(Route) {
    return React.addons.cloneWithProps(Route, {
      routes: this.state.routes,
      linkTo: this.linkTo
    });
  },
  linkTo: function(route, values) {
    return function(e) {
      this.setLocation(this.state.routes[route], values);
      e.preventDefault();
    }.bind(this);
  },
  setLocation: function(route, args) {
    var locationState = this.state.location;
    var context = this.state.context;
    // var locationKey = this.state.locationKey;
    var locationObj = xurl(locationState.href);
    var newLocationState;
    if (this.props.locationHandler) {
      return this.props.locationHandler(locationObj, route);
    } else {
      if (context === HASH_PATH) {
        locationObj.hash = route.href.replace(/:(\w+)/g, (all, name) => args[name]);
      } else {
        console.info('not implemented yet');
      }
      locationState.href = utils.makeURL(locationObj);
      this.setState({location: location});
    }
  },
  getLocation: function() {
    var result;
    var locationState = this.state.location;
    var context = this.state.context;
    // var locationKey = this.state.locationKey;
    var locationObj = xurl(locationState.href);
    if (this.props.locationHandler) {
      result = this.props.locationHandler(locationObj);
    } else {
      if (context === HASH_PATH) {
        result = locationObj.hash.substr(1);
      } else if (context === PATH) {
        result = locationObj.pathname;
      } else {
        console.info('not yet implemented');
      }
    }
    return result || '/';
  },

  render: function() {
    var Route = this.getRouteFromLocation(this.getLocation());
    return this.renderRoute(Route);
  },

});

Router.createClass = function(Rter) {
  return React.createClass({
    render: function() {
      return <Router {...Rter.props} {...this.props} />;
    }
  });
};
