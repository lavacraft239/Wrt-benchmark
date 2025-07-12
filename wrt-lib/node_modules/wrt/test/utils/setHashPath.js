require('../../test-setup');
var assert = require('assert');
var _ = require('lodash');
var utils = require('../../lib/utils');
var setHashPath = utils.setHashPath;

var hashCases = {
  blank: '',
  empty: '#',
  "a hash root": '#/',
};
var rootcases = {
  blank: '',
  "is a slash": '/',
  "has a leading slash": '/root',
  "has a trailing slash": 'root/',
  "has a leading and trailing slash": '/root/',
  "has no leading and trailing slashs": 'root',
};
var pathCases = [
  {
    blank: '',
    slash: '/',
  }, {
    leading: '/new/path',
    trailing: 'new/path/',
    both: '/new/path/',
    trimmed: 'new/path',
  }
];

describe('The setHashPath function', function() {
  _.each(hashCases, function(hash, hashCaseName) {
    describe('when the hash is ' + JSON.stringify(hash), function() {
      _.each(rootcases, function(root, rootCaseName) {
        describe('when the root is ' + JSON.stringify(root), function() {
          _.each(pathCases[0], function(path, pathCaseName) {
            describe('when the path is ' + JSON.stringify(path), function() {
              if (root === '' || root === '/') {
                it('should be http://www.google.com/#/', function() {
                  var url = 'http://www.google.com/' + hash;
                  var newUrl = setHashPath(url, root, path);
                  assert.equal(newUrl, 'http://www.google.com/#/');
                });
              } else {
                it('should be http://www.google.com/#/root/', function() {
                  var url = 'http://www.google.com/' + hash;
                  var newUrl = setHashPath(url, root, path);
                  assert.equal(newUrl, 'http://www.google.com/#/root/');
                });
              }
            });
          });

          _.each(pathCases[1], function(path, pathCaseName) {
            describe('when the path is ' + JSON.stringify(path), function() {
              var noLeadingSlashPath = path.replace(/^\//, '');
              if (root === '' || root === '/') {
                it('should be http://www.google.com/#/' + noLeadingSlashPath, function() {
                  var url = 'http://www.google.com/' + hash;
                  var newUrl = setHashPath(url, root, path);
                  assert.equal(newUrl, 'http://www.google.com/#/' + noLeadingSlashPath);
                });
              } else {
                it('should be http://www.google.com/#/root/' + noLeadingSlashPath, function() {

                  var url = 'http://www.google.com/' + hash;
                  var newUrl = setHashPath(url, root, path);
                  assert.equal(newUrl, 'http://www.google.com/#/root/' + noLeadingSlashPath);
                });
              }
            });
          });
        });
      });
    });
  });
});
