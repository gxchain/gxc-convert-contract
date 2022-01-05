const deliver = artifacts.require('Deliver');

module.exports = function(deployer) {
  deployer.deploy(deliver);
};