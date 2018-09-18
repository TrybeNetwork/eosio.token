/* Title:  trybe_founders_stake.cpp
*  Description: staking contract for Trybe Founders Group
*  Author: Adam Clark
*  Telegram: @aclark80
*/

#include <eosiolib/transaction.hpp>

namespace trybe {

    using std::to_string;

    static constexpr time refund_delay     = 30 * 60;   // hours * mins *  seconds -- 30 minutes for TESTING ONLY
    const uint64_t REQUIRED_STAKE_DURATION = 60;   // 1 hour TESTING ONLY

    /***************************************************************************
    *                               T A B L E S
    ****************************************************************************/

    // @abi table stakedtrybe i64
    struct staked_trybe {
        uint64_t      id;
        account_name  from;
        account_name  to;
        asset         trybe_weight;
        time          time_initial;

        uint64_t  primary_key() const { return id; }

        // explicit serialization macro is not necessary,
        // used here only to improve compilation time
        EOSLIB_SERIALIZE( staked_trybe, (id)(from)(to)(trybe_weight)(time_initial) )
    };

    // @abi table founders i64
    struct founder_table {
        account_name      account;

        uint64_t  primary_key() const { return account; }

        EOSLIB_SERIALIZE( founder_table, (account) )
    };

    // @abi table userres i64
    struct user_resource {
        account_name  owner;
        asset         total_staked_trybe;
        time          initial_staked_time;

        uint64_t primary_key() const { return owner; }

        EOSLIB_SERIALIZE( user_resource, (owner)(total_staked_trybe)(initial_staked_time) )
    };

    typedef multi_index< N(stakedtrybe), staked_trybe>  staked_trybe_table;
    typedef multi_index< N(userres), user_resource>     user_resource_table;
    typedef multi_index< N(founders), founder_table>   founders_table;

    /****************************************************************************
    *                             A C T I O N S
    *****************************************************************************/

    // @abi action
    void token::fndrupdate( account_name founder){

        require_auth2( _self,N(active) );

        eosio_assert( is_account( founder ), "to account does not exist");


        founders_table founder_table( _self, _self );

        auto founder_itr = founder_table.find( founder );

        bool founderactive = check_founder(founder);

        eosio_assert( founderactive == false , "Founder account already added"  );

        founder_table.emplace( founder, [&]( auto& new_entry ){
            new_entry.account = founder;
        });

    }

    // @abi action
    void token::fndrstake( account_name from, account_name receiver, asset total_trybe_to_stake) {

        // Validate passed values

        //Check sender has signed the transaction
        require_auth( from );
        asset current_staked_trybe;

        //Check the receiver is a valid account
        eosio_assert( is_account( receiver ), "receiving account does not exist");

        //Check the amount is valid - CHECK WHAT THIS DOES FULLY
        eosio_assert( total_trybe_to_stake.is_valid(), "invalid asset details offered");

        eosio_assert( total_trybe_to_stake >= asset(0, trybe::SYMBOL), "must stake a positive amount" );

        bool founderactive = check_founder(receiver);

        eosio_assert( founderactive == true , "Account not in founders table"  );

        // QUESTION  Do we want to define a minimum

        user_resource_table user_res(_self, from);
        accounts        from_accounts( _self, from );

        auto from_trybe_account = from_accounts.find( total_trybe_to_stake.symbol.name() );
        auto user_resource_itr  = user_res.find( from );



        if ( user_resource_itr == user_res.end() )
            current_staked_trybe = asset(0, SYMBOL);
        else
            current_staked_trybe = user_resource_itr->total_staked_trybe;

        eosio_assert( total_trybe_to_stake < (from_trybe_account->balance - current_staked_trybe ),
                      "not enough liquid TRYBE to stake" );


        stake_trybe( from, receiver, total_trybe_to_stake );
        update_user_resources( from, total_trybe_to_stake );

    }

    // @abi action
    void token::fndrunstake( account_name from, uint64_t stake_id ) {
        require_auth( from );

        staked_trybe_table staked_index( _self, from );

        auto unstake_itr = staked_index.find( stake_id );

        eosio_assert( unstake_itr != staked_index.end(), "staked row does not exist");

    }


    /****************************************************************************
    *                             F U N C T I O N S
    *****************************************************************************/

    bool token::check_founder( account_name founder ){

        founders_table founder_table( _self, _self );

        auto founder_itr = founder_table.find( founder );

        if ( founder_itr == founder_table.end() ){
            return false;
        } else{
            return true;
        }


    }

    asset inline token::get_staked_trybe( account_name account ){

        user_resource_table user_res(_self, account);

        auto user_resource_itr        = user_res.find( account );

        if ( user_resource_itr == user_res.end() )
            return asset(0, SYMBOL);
        else
            return user_resource_itr->total_staked_trybe;

    }

    void inline token::stake_trybe( account_name& from,
                                              account_name& receiver,
                                              const asset&  stake_trybe_delta ) {

        staked_trybe_table staked_index( _self, from );

        // Insert a new row for each stake
        auto itr = staked_index.emplace( from /* staker owns RAM */, [&]( auto& stake ){
            stake.id            = staked_index.available_primary_key();
            stake.from          = from;
            stake.to            = receiver;
            stake.trybe_weight  = stake_trybe_delta;
            stake.time_initial  = now();
        });

        eosio_assert( stake_trybe_delta == itr->trybe_weight, "staking TRYBE failed" );
    }

    void inline token::update_user_resources( account_name& owner,
                                                     const asset&  stake_trybe_delta ) {
        user_resource_table totals_tbl( _self, owner );

        auto tot_itr = totals_tbl.find( owner );

        if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( owner, [&]( auto& tot ) {
                tot.owner                 = owner;
                tot.total_staked_trybe    = stake_trybe_delta;
                tot.initial_staked_time   = now();
            });
        } else {
            totals_tbl.modify( tot_itr, 0, [&]( auto& tot ) {
                tot.total_staked_trybe    += stake_trybe_delta;
            });
        }
        eosio_assert( asset(0, SYMBOL) <= tot_itr->total_staked_trybe, "insufficient staked total TRYBE" );

        if ( tot_itr->total_staked_trybe == asset(0, SYMBOL) ) {
            totals_tbl.erase( tot_itr );
        }
    };

}  /// namespace trybe
