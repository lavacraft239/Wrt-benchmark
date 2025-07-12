var React = require('react');
var xURL = require('xurl');

exports.makeURL = makeURL;
exports.setPath = setPath;
exports.setHashPath = setHashPath;
// exports.setParam = setParam;
// exports.setSearchParam = setSearchParam;
// exports.setHashParam = setHashParam;
exports.isReactClass = isReactClass;

function makeURL(parts) {
  return (
    (parts.protocol ? parts.protocol + '//' : '') +
    parts.host +
    parts.pathname +
    parts.search +
    parts.hash
  );
}


function setPath(url, basePath, newPath) {
  var parts = xURL(url);
  basePath = basePath || '/';
  if (basePath !== '/') {
    basePath = '/' + basePath.replace(/^\/|\/$/g, '') + '/';
  }
  newPath = newPath.replace(/^\//, '');
  parts.pathname = basePath + newPath;
  return makeURL(parts);
}

function setHashPath(url, baseHashPath, newHashPath) {
  var parts = xURL(url);
  baseHashPath = baseHashPath || '/';
  if (baseHashPath !== '/') {
    baseHashPath = '/' + baseHashPath.replace(/^\/|\/$/g, '') + '/';
  }
  newHashPath = newHashPath.replace(/^\//, '');
  parts.hash = '#' + baseHashPath + newHashPath;
  return makeURL(parts);
}

function setParam(queryString, param, value, anchor) {
  var regex = RegExp('((?:' + anchor + '|[&])' + param + '=)([^&]*)');
  var prefix = queryString.length > 1 ? '&' : '';
  return regex.test(queryString) ?
    queryString.replace(regex, '$1' + value) :
    queryString + prefix + param + '=' + value;
}

function setSearchParam(url, param, value) {
  var parts = xURL(url);
  return (
    parts.pathname +
    setParam(parts.search, param, value, '\\?') +
    parts.hash
  );
}

function setHashParam(url, param, value) {
  var parts = xURL(url);
  return (
    parts.pathname +
    parts.search +
    setParam(parts.hash, param, value, '#')
  );
}

var DummyClass = React.createClass({render:function() {}});
function isReactClass(fn) {
  return fn.toString() === DummyClass.toString();
}
