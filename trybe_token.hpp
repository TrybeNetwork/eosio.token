#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

namespace trybe {

    using namespace eosio;
    using eosio::asset;
    using std::string;

    static uint64_t     SYMBOL = string_to_symbol(4, "TRYBE");
    static int64_t      MAX_SUPPLY = 10'000'000'000'0000;

    class token : public contract {
    public:
        token( account_name self ):contract(self){}

        /***************************************************************************
       *                               ACTIONS
       ****************************************************************************/

        void create();

        void claim( account_name claimer );

        void issue( account_name to,
                    asset quantity,
                    string memo );

        void transfer( account_name from,
                       account_name to,
                       asset        quantity,
                       string       memo );

        void fndrstake( account_name from,
                             account_name receiver,
                             asset total_trybe_to_stake);

        void fndrunstake( account_name from,
                                uint64_t stake_id );

        void fndrupdate( account_name founder);


        inline asset get_supply( symbol_name sym )const;

        inline asset get_balance( account_name owner,
                                  symbol_name sym )const;

    private:

      /***************************************************************************
      *                               Functions
      ****************************************************************************/


        void inline stake_trybe( account_name& from,
                                           account_name& receiver,
                                           const asset&  stake_trybe_delta );

        void inline update_user_resources( account_name& owner, const asset&  stake_trybe_delta );

        void sub_balance( account_name owner, asset value );

        void add_balance( account_name owner, asset value, account_name ram_payer );

        asset get_staked_trybe(account_name account);

        bool check_founder( account_name founder );

        /***************************************************************************
        *                               T A B L E S
        ****************************************************************************/

        // @abi table accounts i64
        struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }

            EOSLIB_SERIALIZE( account, (balance) )
        };

        // @abi table stat i64
        struct currency_stat {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }

            EOSLIB_SERIALIZE( currency_stat, (supply)(max_supply)(issuer) )
        };

        typedef eosio::multi_index<N(accounts), account> accounts;
        typedef eosio::multi_index<N(stat), currency_stat> stats;

        /***************************************************************************
       *                           END T A B L E S
       ****************************************************************************/


    public:

    };

    asset token::get_supply( symbol_name sym )const
    {
        stats statstable( _self, sym );
        const auto& st = statstable.get( sym );
        return st.supply;
    }

    asset token::get_balance( account_name owner, symbol_name sym )const
    {
        accounts accountstable( _self, owner );
        const auto& ac = accountstable.get( sym );
        return ac.balance;
    }

}
