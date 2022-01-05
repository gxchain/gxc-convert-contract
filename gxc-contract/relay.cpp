#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <graphenelib/dispatcher.hpp>
#include <vector>

using namespace graphene;

class relay : public contract
{
public:
    relay(uint64_t account_id)
        : contract(account_id), fund_in_table(_self, _self), eth_confirm_table(_self, _self), nonce_table(_self, _self), coin_table(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void deposit(std::string target, std::string addr)
    {
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        graphene_assert(asset_id != -1, "Invalid Asset");
        auto asset_itr = coin_table.find(asset_id);
        graphene_assert(asset_itr != coin_table.end(), "The asset is not supported");
        graphene_assert(asset_itr->enable_transfer == 1, "The asset is locked");
        graphene_assert(asset_amount >= asset_itr->min_deposit, "Must greater than minnumber ");
        uint64_t id_number = fund_in_table.available_primary_key();
        graphene_assert(target == "REI", "Invalid chain name");
        uint64_t sender = get_trx_sender();
        fund_in_table.emplace(sender, [&](auto &o)
                              {
                                  o.id = id_number;
                                  o.from = sender;
                                  o.asset_id = asset_id;
                                  o.amount = asset_amount;
                                  o.target = target;
                                  o.to = addr;
                                  o.state = 0;
                              });
    }

    // @abi action
    // @abi payable
    void deposit2(std::string target, std::string addr, std::string nonce)
    {
        uint64_t sender = get_trx_sender();
        graphene_assert(sender == adminAccount, "You have no authority");
        for (auto id_begin = nonce_table.begin(); id_begin != nonce_table.end(); id_begin++)
        {
            graphene_assert((*id_begin).nonce != nonce, "The nonce is existed");
        }
        deposit(target, addr);
        auto id_number = nonce_table.available_primary_key();
        nonce_table.emplace(sender, [&](auto &o)
                            {
                                o.id = id_number;
                                o.nonce = nonce;
                            });
        auto begin_iterator = nonce_table.begin();
        if (id_number - (*begin_iterator).id > NONCE_LIMIT)
        {
            nonce_table.erase(begin_iterator);
        }
    }

    //@abi action
    void confirmd(uint64_t order_id, std::string target, std::string addr, contract_asset amount, std::string txid)
    {
        uint64_t sender = get_trx_sender();
        graphene_assert(sender == adminAccount, "You have no authority");
        auto idx = fund_in_table.find(order_id);
        graphene_assert(idx != fund_in_table.end(), "There is no that order_id");
        graphene_assert((*idx).target == target, "Unmatched chain name");
        graphene_assert((*idx).asset_id == amount.asset_id, "Unmatched assert id");
        graphene_assert((*idx).amount == amount.amount, "Unmatched assert amount");
        graphene_assert(target == "REI", "Invalid chain name, only support REI so far");
        if (target == "REI")
        {
            for (auto id_begin = eth_confirm_table.begin(); id_begin != eth_confirm_table.end(); id_begin++)
            {
                graphene_assert((*id_begin).txid != txid, "The txid is existed, be honest");
            }
            auto id_number = eth_confirm_table.available_primary_key();
            eth_confirm_table.emplace(sender, [&](auto &o)
                                      {
                                          o.id = id_number;
                                          o.txid = txid;
                                      });
            auto begin_iterator = eth_confirm_table.begin();
            if (id_number - (*begin_iterator).id > TXID_LIST_LIMIT)
            {
                eth_confirm_table.erase(begin_iterator);
            }
            fund_in_table.modify(idx, sender, [&](auto &o)
                                 { o.state = 1; });
            fund_in_table.erase(idx);
        }
    }

    //@abi action
    void adjustcoin(std::string coinname, uint64_t enabletransfer, uint64_t mindeposit)
    {
        uint64_t sender = get_trx_sender();
        graphene_assert(sender == adminAccount, "You have no authority");
        auto asset_id = get_asset_id(coinname.c_str(), coinname.size());
        graphene_assert(asset_id != -1, "Invalid Asset");
        auto asset_itr = coin_table.find(asset_id);
        if (asset_itr == coin_table.end())
        {
            coin_table.emplace(sender, [&](auto &o)
                               {
                                   o.asset_id = asset_id;
                                   o.enable_transfer = enabletransfer;
                                   o.min_deposit = mindeposit;
                               });
        }
        else
        {
            coin_table.modify(asset_itr, sender, [&](auto &o)
                              {
                                  o.enable_transfer = enabletransfer;
                                  o.min_deposit = mindeposit;
                              });
        }
    }

private:
    const uint64_t adminAccount = 1565943; // gxc-convert
    const uint64_t TXID_LIST_LIMIT = 100;
    const uint64_t NONCE_LIMIT = 10;

    //@abi table nonceids i64
    struct nonceids
    {
        uint64_t id;
        std::string nonce;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(nonceids, (id)(nonce))
    };
    typedef multi_index<N(nonceids), nonceids> nonceids_index;

    //@abi table ctxids i64
    struct ctxids
    {
        uint64_t id;
        std::string txid;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(ctxids, (id)(txid))
    };
    typedef multi_index<N(ctxids), ctxids> ctxids_index;

    //@abi table fundin i64
    struct fundin
    {
        uint64_t id;
        uint64_t from;
        uint64_t asset_id;
        int64_t amount;
        std::string target;
        std::string to;
        uint64_t state;

        uint64_t primary_key() const { return id; }
        uint64_t by_sender() const { return from; }

        GRAPHENE_SERIALIZE(fundin, (id)(from)(asset_id)(amount)(target)(to)(state))
    };

    typedef multi_index<N(fundin), fundin,
                        indexed_by<N(sender), const_mem_fun<fundin, uint64_t, &fundin::by_sender>>>
        fund_in_index;

    //@abi table coin i64
    struct coin
    {
        uint64_t asset_id;
        uint64_t enable_transfer;
        uint64_t min_deposit;
        uint64_t primary_key() const { return asset_id; }

        GRAPHENE_SERIALIZE(coin, (asset_id)(enable_transfer)(min_deposit))
    };
    typedef multi_index<N(coin), coin> coin_index;

    fund_in_index fund_in_table;
    ctxids_index eth_confirm_table;
    nonceids_index nonce_table;
    coin_index coin_table;
};

GRAPHENE_ABI(relay, (deposit)(deposit2)(confirmd)(adjustcoin))