#pragma once

#include <iostream>
#include "wallet/common.h"
#include <boost/msm/front/functor_row.hpp>

namespace beam::wallet
{
    class Sender : public FSMHelper<Sender>
    {
    public:
        // interface to communicate with receiver
        struct InvitationData
        {
            Uuid m_txId;
            ECC::Amount m_amount; ///??
            ECC::Hash::Value m_message;
            ECC::Point::Native m_publicSenderBlindingExcess;
            ECC::Point::Native m_publicSenderNonce;
            std::vector<Input> m_inputs;
          //  std::vector<Output> m_outputs;
        };

        struct ConfirmationData
        {
            Uuid m_txId;
            ECC::Scalar::Native m_senderSignature;
        };

        struct IGateway
        {
            virtual void sendTxInitiation(const InvitationData&) = 0;
            virtual void sendTxConfirmation(const ConfirmationData&) = 0;
            virtual void sendChangeOutputConfirmation() = 0;
        };

        // events
        struct TxEventBase {};
        struct TxInitCompleted : TxEventBase {};
        struct TxInitFailed : TxEventBase {};
        struct TxConfirmationCompleted : TxEventBase {};
        struct TxConfirmationFailed : TxEventBase {};
        struct TxOutputConfirmCompleted : TxEventBase {};
        struct TxOutputConfirmFailed : TxEventBase {};
        
        Sender(IGateway& gateway, const Uuid& txId)
            : m_fsm{boost::ref(gateway), boost::ref(txId)}
        {
            
        }    
    private:
        struct FSMDefinition : public msmf::state_machine_def<FSMDefinition>
        {
            // states
            struct Init : public msmf::state<> {
                template <class Event, class Fsm>
                void on_entry(Event const&, Fsm&)
                { std::cout << "[Sender] Init state\n"; } };
            struct Terminate : public msmf::terminate_state<> {
                template <class Event, class Fsm>
                void on_entry(Event const&, Fsm&)
                { std::cout << "[Sender] Terminate state\n"; } };
            struct TxInitiating : public msmf::state<> {
                template <class Event, class Fsm>
                void on_entry(Event const&, Fsm&)
                { std::cout << "[Sender] TxInitiating state\n"; } };
            struct TxConfirming : public msmf::state<> {
                template <class Event, class Fsm>
                void on_entry(Event const&, Fsm&)
                { std::cout << "[Sender] TxConfirming state\n"; } };
            struct TxOutputConfirming : public msmf::state<> {
                template <class Event, class Fsm>
                void on_entry(Event const&, Fsm&)
                { std::cout << "[Sender] TxOutputConfirming state\n"; } };

            FSMDefinition(IGateway& gateway, const Uuid& txId)
                : m_gateway{ gateway }
                , m_txId{txId}
            {}

            // transition actions
            void initTx(const msmf::none&)
            {
                m_invitationData.m_txId = m_txId;
                m_gateway.sendTxInitiation(m_invitationData);
            }

            bool isValidSignature(const TxInitCompleted& event)
            {
                std::cout << "Sender::isValidSignature\n";
                return true;
            }

            bool isInvalidSignature(const TxInitCompleted& event)
            {
                std::cout << "Sender::isInvalidSignature\n";
                return false;
            }

            void confirmTx(const TxInitCompleted& event)
            {
                m_confirmationData.m_txId = m_txId;
                m_gateway.sendTxConfirmation(m_confirmationData);
            }

            void rollbackTx(const TxInitFailed& event)
            {
                std::cout << "Sender::rollbackTx\n";
            }

            void rollbackTx(const TxConfirmationFailed& event)
            {
                std::cout << "Sender::rollbackTx\n";
            }
            
            void rollbackTx(const TxOutputConfirmFailed& event)
            {
                std::cout << "Sender::rollbackTx\n";
            }

            void cancelTx(const TxInitCompleted& event)
            {
                std::cout << "Sender::cancelTx\n";
            }

            void confirmChangeOutput(const TxConfirmationCompleted& event)
            {
                m_gateway.sendChangeOutputConfirmation();
            }

            void completeTx(const TxOutputConfirmCompleted& event)
            {
                std::cout << "Sender::completeTx\n";
            }

            using initial_state = Init;
            using d = FSMDefinition;
            struct transition_table : mpl::vector<
                //   Start                 Event                     Next                  Action                    Guard
                a_row< Init              , msmf::none              , TxInitiating        , &d::initTx                                      >,
                a_row< TxInitiating      , TxInitFailed            , Terminate           , &d::rollbackTx                                  >,
                row  < TxInitiating      , TxInitCompleted         , TxConfirming        , &d::confirmTx           , &d::isValidSignature  >,
                row  < TxInitiating      , TxInitCompleted         , Terminate           , &d::cancelTx            , &d::isInvalidSignature>,
                a_row< TxConfirming      , TxConfirmationCompleted , TxOutputConfirming  , &d::confirmChangeOutput                         >,
                a_row< TxConfirming      , TxConfirmationFailed    , Terminate           , &d::rollbackTx                                  >,
                a_row< TxOutputConfirming, TxOutputConfirmCompleted, Terminate           , &d::completeTx                                  >,
                a_row< TxOutputConfirming, TxOutputConfirmFailed   , Terminate           , &d::rollbackTx                                  >
            > {};

            template <class FSM, class Event>
            void no_transition(Event const& e, FSM&, int state)
            {
                std::cout << "[Sender] no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
            }

            IGateway& m_gateway;
            Uuid m_txId;
            InvitationData m_invitationData;
            ConfirmationData m_confirmationData;
        };
    protected:
        friend FSMHelper<Sender>;
        msm::back::state_machine<FSMDefinition> m_fsm;
    };
}