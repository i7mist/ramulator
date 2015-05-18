#include "Controller.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"

using namespace ramulator;

namespace ramulator {

static vector<int> get_offending_subarray(DRAM<SALP>* channel, vector<int> & addr_vec){
    int sa_id = 0;
    auto rank = channel->children[addr_vec[int(SALP::Level::Rank)]];
    auto bank = rank->children[addr_vec[int(SALP::Level::Bank)]];
    auto sa = bank->children[addr_vec[int(SALP::Level::SubArray)]];
    for (auto sa_other : bank->children)
        if (sa != sa_other && sa_other->state == SALP::State::Opened){
            sa_id = sa_other->id;
            break;
        }
    vector<int> offending = addr_vec;
    offending[int(SALP::Level::SubArray)] = sa_id;
    offending[int(SALP::Level::Row)] = -1;
    return offending;
}


template <>
vector<int> Controller<SALP>::get_addr_vec(SALP::Command cmd, list<Request>::iterator req){
    if (cmd == SALP::Command::PRE_OTHER)
        return get_offending_subarray(channel, req->addr_vec);
    else
        return req->addr_vec;
}


template <>
bool Controller<SALP>::is_ready(list<Request>::iterator req){
    SALP::Command cmd = get_first_cmd(req);
    if (cmd == SALP::Command::PRE_OTHER){

        vector<int> addr_vec = get_offending_subarray(channel, req->addr_vec);
        return channel->check(cmd, addr_vec.data(), clk);
    }
    else return channel->check(cmd, req->addr_vec.data(), clk);
}

template <>
void Controller<ALDRAM>::update_temp(ALDRAM::Temp current_temperature){
    channel->spec->aldram_timing(current_temperature);
}


template <>
void Controller<TLDRAM>::tick(){
    clk++;

    /*** 1. Serve completed reads ***/
    if (pending.size()) {
        Request& req = pending[0];
        if (req.depart <= clk) {
            req.callback(req);
            pending.pop_front();
        }
    }

    /*** 2. Should we schedule refreshes? ***/
    refresh->tick_ref();

    /*** 3. Should we schedule writes? ***/
    if (!write_mode) {
        // yes -- write queue is almost full or read queue is empty
        if (writeq.size() >= int(0.8 * writeq.max) || readq.size() == 0)
            write_mode = true;
    }
    else {
        // no -- write queue is almost empty and read queue is not empty
        if (writeq.size() <= int(0.2 * writeq.max) && readq.size() != 0)
            write_mode = false;
    }

    /*** 4. Find the best command to schedule, if any ***/
    Queue* queue = !write_mode ? &readq : &writeq;
    if (otherq.size())
        queue = &otherq;  // "other" requests are rare, so we give them precedence over reads/writes

    auto req = scheduler->get_head(queue->q);
    if (req == queue->q.end() || !is_ready(req)) {
        // we couldn't find a command to schedule -- let's try to be speculative
        auto cmd = TLDRAM::Command::PRE;
        vector<int> victim = rowpolicy->get_victim(cmd);
        if (!victim.empty()){
            issue_cmd(cmd, victim);
        }
        return;  // nothing more to be done this cycle
    }

    /*** 5. Change a read request to a migration request ***/
    if (req->type == Request::Type::READ) {
        req->type = Request::Type::EXTENSION;
    }

    // issue command on behalf of request
    auto cmd = get_first_cmd(req);
    issue_cmd(cmd, get_addr_vec(cmd, req));

    // check whether this is the last command (which finishes the request)
    if (cmd != channel->spec->translate[int(req->type)])
        return;

    // set a future completion time for read requests
    if (req->type == Request::Type::READ || req->type == Request::Type::EXTENSION) {
        req->depart = clk + channel->spec->read_latency;
        pending.push_back(*req);
    }

    // remove request from queue
    queue->q.erase(req);
}

} // namespace ramulator
