require('../../test-setup');
var assert = require('assert');
var React = require('react');
var wrt = require('../..');

var Router = wrt.Router;
var Route = wrt.Route;

describe('A basic hash router', function() {
  describe('when embedded directly on the DOM', function() {

    describe('when the location is "http://www.google.com/"', function() {
      var routerElement;
      var router;

      beforeEach(function() {
        routerElement = (
          <Router root="#/" location={{href:'http://www.google.com/'}}>
            <Route path="/" handler={function() { return <h1>Root</h1>; }} />
            <Route path="/sub" handler={function() { return <h2>Sub</h2>; }} />
          </Router>
        );
        router = React.render(routerElement, document.body);
      });

      afterEach(function(done) {
        React.unmountComponentAtNode(document.body);
        setTimeout(done);
      });

      it('should default to the root route', function() {
        assert.equal(router.getDOMNode().textContent, 'Root');
      });

      it('should change routes when the location changes', function() {
        var newLocation = { href: 'http://www.google.com/#/sub' };
        router.setState({location: newLocation});
        assert.equal(router.getDOMNode().textContent, 'Sub');
      });

    });

    describe('when the location is "http://www.google.com/#/sub"', function() {
      var routerElement;
      var router;

      beforeEach(function() {
        routerElement = (
          <Router root="#/" location={{href:'http://www.google.com/#/sub'}}>
            <Route path="/" handler={function() { return <h1>Root</h1>; }} />
            <Route path="/sub" handler={function() { return <h2>Sub</h2>; }} />
          </Router>
        );
        router = React.render(routerElement, document.body);
      });

      afterEach(function(done) {
        React.unmountComponentAtNode(document.body);
        setTimeout(done);
      });

      it('should go to the sub route', function() {
        assert.equal(router.getDOMNode().textContent, 'Sub');
      });

      it('should change routes when the location changes', function() {
        var newLocation = { href: 'http://www.google.com/#/' };
        router.setState({location: newLocation});
        assert.equal(router.getDOMNode().textContent, 'Root');
      });

    });

  });

});

