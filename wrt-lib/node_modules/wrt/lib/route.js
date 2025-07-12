var React = require('react');
var utils = require('./utils');

module.exports = React.createClass({
  render: function() {
    var R = this.props.handler;
    var props = this.props;
    var routes = this.props.routes;

    if (utils.isReactClass(R)) {
      return <R {...props} />
    } else {
      return R(props);
    }
  }
});
