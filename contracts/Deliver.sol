pragma solidity 0.6.2;

import "@openzeppelin/contracts/access/AccessControl.sol";
import "@openzeppelin/contracts/utils/Pausable.sol";
import "@openzeppelin/contracts/utils/ReentrancyGuard.sol";

contract Deliver is AccessControl, Pausable, ReentrancyGuard {
    struct TokenSettings {
        uint256 minDeliver;
    }

    bytes32 public constant MANAGER_ROLE = keccak256("MANAGER_ROLE");
    bytes32 public constant PAUSER_ROLE = keccak256("PAUSER_ROLE");
    bytes32 public constant DELIVER_ROLE = keccak256("DELIVER_ROLE");

    TokenSettings public REISettings;

    mapping(bytes32 => bool) public txidMap;

    event DeliverREI(address indexed to, uint256 amount, string from, bytes32 txid);

    modifier checkTxid(bytes32 txid) {
        require(txidMap[txid] == false, "Invalid txid.");
        txidMap[txid] = true;
        _;
    }

    constructor() public {
        _setupRole(DEFAULT_ADMIN_ROLE, _msgSender());
        _setupRole(MANAGER_ROLE, _msgSender());
        _setupRole(PAUSER_ROLE, _msgSender());
    }

    function deliverREI(address payable to, uint256 amount, string calldata from, bytes32 txid) external whenNotPaused checkTxid(txid) {
        require(hasRole(DELIVER_ROLE, _msgSender()), "Invalid sender.");
        require(REISettings.minDeliver <= amount, "Insufficient amount.");
        to.transfer(amount);
        emit DeliverREI(to, amount, from, txid);
    }

    fallback() external payable {}

    function pause() public {
        require(hasRole(PAUSER_ROLE, _msgSender()), "Invalid sender.");
        _pause();
    }
    function unpause() public {
        require(hasRole(PAUSER_ROLE, _msgSender()), "Invalid sender.");
        _unpause();
    }

    function manageREISettings( uint256 minDeliver) external {
        require(hasRole(MANAGER_ROLE, _msgSender()), "Invalid sender.");
        REISettings.minDeliver = minDeliver;
    }
}