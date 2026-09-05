#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::steady_clock; // whenever i type clock, this is what it really means

// ------------------------- Events -------------------------
struct Tick {           //a tick represents a market data update

    int64_t seq = 0;           //  sequence number (monotonically increasing)
    //                          int64_t so its large enough to never overflow
    double price = 0.0;       
    Clock::time_point created_at{};     //time stamp
};

enum class Side { Buy, Sell };      //list of possible options: side can only be buy or sell 

struct OrderRequest {
    int64_t id = 0;
    Side side = Side::Buy;          // here because we decide intent

    int qty = 0;                    //quantity to trade
    //              market order, so no limit price    
    Clock::time_point created_at{};  // time stamp
};

struct Fill {
    int64_t order_id = 0;
    Side side = Side::Buy;
    int qty = 0;
    double price = 0.0;     //execution price
    Clock::time_point created_at{};
};

using Event = variant<Tick, OrderRequest, Fill>;    

// ------------------------- Queue -------------------------
class EventQueue {      //a mailbox for events: where we store events until they get processed by engine
public:
    void push(Event e) { 
        q_.push_back(move(e));
    }

    bool empty() const { 
        return q_.empty(); 
    }

    optional<Event> pop() {            //returns an event (optionnally ==> c'est pas sur que ca retourne haga aslan) that's why we dont just put event car whatif nothing is returned

        if (q_.empty()) return nullopt;    // if theres nothing to pop on the queue

        Event e = move(q_.front());        // moves e to front mahma kan fen: when we sa

        q_.pop_front();         // pop_front() removes the first element of a container (here q_) and destroys it

        return e;
    }

private:
    deque<Event> q_;       // definition of q_
};


// ------------------------- Metrics -------------------------
struct Metrics {

    //how fast is the engine??      → throughput (events per second)
    // throughput= events_processed / total_time

    //How long does an event take to be processed?      → latency   (creation → handling)
    // latency_ns = time_when_processed − time_when_created


    uint64_t events_processed = 0;     //a counter of events processed by engine 
    // will be incremented every time engine pops an event from queue


    vector<int64_t> lat_ns;       //latency vector en ns
    // one latency measurement per event
    
    //why vector???     to calculate moyenne et mediane de latency but also to answer:
        //What does a typical event look like?
        //How bad is the worst-case?
        //Are there long tails?
    



    //  using the clock alias creeated eralier
    Clock::time_point start = Clock::now();       //creates a variable called start whose type is time_point   
    // a stopwatch timestamp

    // At the moment the Metrics object is created:    "   Metrics m,   "       
    // This line executes and: captures the current stopwatch value and stores it in start
    
    
    Clock::time_point end = start;      //on intialize a end=start so that start-end=0 a t=0    , end would be continuously incremented



    void record_latency(Clock::time_point created_at) {

        // it measures end-to-end latency: the time between when an event was created and when it is handled by the engine

        //need to input when it was created

        auto now = Clock::now();        //the moment the engine is processing the event

        auto elapsed = now - created_at; //given as argument
        auto elapsed_ns =
            chrono::duration_cast<chrono::nanoseconds>(elapsed);
        auto ns = elapsed_ns.count();

        //  duration_cast<chrono::nanoseconds>:    to convert to ns

        // now - created_at is a    Clock::duration (its not just a number , so...)
        // the .count() extracts the number (= the duration)


        lat_ns.push_back(ns);       //adds value to the latency vector
    }

    void finish() { 
        // records the exact moment the simulation finishes so total runtime (wall time) can be computed.
        end = Clock::now(); //re affectation de end qui est definie en haut (initialisé a =start)
    }



    void print_summary() const {

        auto dur_ns = chrono::duration_cast<chrono::nanoseconds>(end - start).count(); //duration en ns (again) (check latency function for exp)
        double seconds = dur_ns / 1e9; // same duration but en s

        double throughput = 0.0; //init
        if (seconds > 0.0) {
            throughput = events_processed / seconds;        //events_processed defined globally en haut
        }



        cout << "\n xxxxxxxxxxxx metrics xxxxxxxxxxxx\n";         //cout is the standard output stream in C++
        cout << "numb of events processed: " << events_processed << "\n";
        cout << "stopwatch (duration totale en s): " << seconds << "\n";
        cout << "throughput (events/s): " << throughput << "\n";

        if (!lat_ns.empty()) {      // NOT using empty() method defined before, but the same predefined empty() used in that method's logic

            

            long long sum = 0;
            for (auto v : lat_ns) {
                sum += v;
            }

            double avg_latency = static_cast<double>(sum) / lat_ns.size();



            auto sorted = lat_ns; //copy of latency vector, so we dont mix up original order

            sort(sorted.begin(), sorted.end()); //sorts values from smallest to largest:       sorted[0] → fastest event latency

            auto p50 = sorted[sorted.size() / 2]; // hat latency bel sor3a el mid (50% aktar w 50% a2al) ==> median latency

            auto p95 = sorted[(sorted.size() * 95) / 100]; //indey of 95th percentile:  "A latency that 95% of events did not exceed"
            //Only 5% of events were slower than this

            auto p99 = sorted[(sorted.size() * 99) / 100]; //What do the slowest 1% of events look like? == "TAIL LATENCY"

            

            cout << "Latency ns:\n" 
                    << "average latency=" << avg_latency<< "ns \n"
                    << " p50=" << p50<< "\n"
                    << " p95=" << p95<< "\n"
                    << " p99=" << p99 << "\n";        //prints values


            
        }
    }
};

// ------------------------- Market Data Feed (Sim) -------------------------

class MarketDataFeed {
public:
    MarketDataFeed(double start_price, unsigned seed)       //must give these 2 values to create an object: a starting price AND a seed (how randomness start)

        : price_(start_price), rng_(seed), dist_(0.0, 0.2) {} //Set the internal price to the starting price;
        //Create the random number generator using this seed
       // et up the randomness so price changes are small and centered around 0

    Tick next_tick() {

        // generates one new market price update -- method that returns variable of type tick (defined fo2 khales)
        //when called gives you new tick

        price_ += dist_(rng_);      // dist here takes 1 argument to generate a value //lessa mesh mafhouma awi

        if (price_ < 1.0) price_ = 1.0; //because negative or zero prices would break the simulation --> PnL math would stop making sense

        Tick t; //create object 
        t.seq = seq_++;
        t.price = price_; //give it the price
        t.created_at = Clock::now(); //stamp time
        return t;
    }

private:
    double price_; 
    int64_t seq_ = 0; //number of tick: "tick number 1, number 2...."
    mt19937 rng_;
    normal_distribution<double> dist_;  //normal_distribution is a predefined type to generate number in normal dist (comes from RANDOM)
    // produces numbers that look like small random changes based on 2 arguments given (mean, spread: controls how big price moves are == volatility)

};


// ------------------------- Strategy -------------------------
class SimpleMomentumStrategy {
public:
    // If price moves up/down more than threshold since last tick, trade.

    //it reacts tick-by-tick, comparing the current price to the previous one.

    optional<OrderRequest> on_tick(const Tick& t) {        //can return order request or not, we dk
        
        //takes as input a tick
        optional<OrderRequest> out; //optional = empty by default until affected

        if (last_price_.has_value()) {              //cuz can only compute if theres a last tick to compare to
            //                                  if theres no last price, we skip the if logic --> this one becomes last directly

            double diff = t.price - *last_price_;           //différence = momentum
            // we use * cause its not a double... its an optional double --> gives u whats inside the box

            if (diff > threshold_) {
                out = OrderRequest{next_order_id_++, Side::Buy, qty_, Clock::now()};

                //RAPPEL: order request struct: ID, side, quantity, time stamp

            } else if (diff < -threshold_) {
                out = OrderRequest{next_order_id_++, Side::Sell, qty_, Clock::now()};
            }
        }

        last_price_ = t.price; //                                               <- |
        return out;
    }

private:
    optional<double> last_price_; //stores price of previous tick          -> |
    double threshold_ = 0.15; // minimum price movement required to trigger a trade  ARBITRARY
    //                          If price increases by more than 0.15 → Buy
    //                          If price decreases by more than 0.15 → Sell
    int qty_ = 10;      //order size
    int64_t next_order_id_ = 1; // id (incremented) 
};


// ------------------------- Risk Checks -------------------------

class RiskChecks {                                      //      “Is this order allowed?”
public:
// only case where its not allowed is when exeeding max quanitty

    bool allow(const OrderRequest& o) const {                   //flag system 
        // takes order request as input (struct: ID, side, quantity, time stamp)

        if (o.qty <= 0) return false;
        if (o.qty > max_qty_) return false;
        return true;
    }

private:
    int max_qty_ = 100;     // Maximum allowed quantity per order. hard coded here for simplicity but normally varies 3alatoul
};

// ------------------------- Execution Simulator -------------------------
class ExecutionSimulator {                  // takes an order and returns a fill (copies most)
public:

    optional<Fill> execute(const OrderRequest& o, double last_price) {        

        //takes as input order request and last_price
        
        Fill f;             // object of type fill defined fel awel (strcut= id, side, quantitiy, price, time)
        f.order_id = o.id;
        f.side = o.side;
        f.qty = o.qty;
        f.price = last_price;
        f.created_at = Clock::now();
        return f;
    }
};

// ------------------------- Portfolio -------------------------
class Portfolio {
public:

    void on_fill(const Fill& f) {
        
        //updates the portfolio when a trade actually happens 
        // based on inputted fill (strcut= id, side, quantitiy, price, time)


        double signed_qty;
        if (f.side == Side::Buy) {
            signed_qty = f.qty;
        } else {
            signed_qty = -f.qty;
        }


        position_ += signed_qty; //adds quantity to position (pos or neg)

        cash_ -= signed_qty * f.price; // updates cash balance = how much bought/sold (neg or pos)      *       price of stock at moment

        last_price_ = f.price;          
    }


    double pnl_mtm() const {        //what my portfolio's worth

        //      = cash + liquidation of stocks (value of holdings)
        
        return position_ * last_price_ + cash_;
    }



    void print_final() const {
        cout << "\nxxxxxxxxxxxx Portfolio xxxxxxxxxxxxn"<< "\n";
        cout << "position: " << position_ << "\n";
        cout << "cash:     " << cash_ << "\n";
        cout << "lastPx:   " << last_price_ << "\n";
        cout << "PnL(MTM): " << pnl_mtm() << "\n";
    }


private:
    double position_ = 0.0; //how much you own of this stock
    double cash_ = 0.0; //how much cash you have
    double last_price_ = 0.0; //the latest market price of this stock 
};

// ------------------------- Main / Engine Loop -------------------------
int main() {
    const int num_ticks = 10;                                                                        // MODFIY
    //how many tick simulations 
    const unsigned seed = 11;                                                      // MODIFY (SEEED)
    //KEEPING SAME SEED WILL MAKE U GET SAME RESULTS

    EventQueue q;       //create the queue
    MarketDataFeed feed(100.0, seed);                                                                //MODIFY
    //create simulator:     takes as argument start price= 100 and seed (inputted)
    SimpleMomentumStrategy strat;
    RiskChecks risk;
    ExecutionSimulator exec;
    Portfolio port;
    Metrics m;


    double last_price = 100.0;                       //initialized as first price

    //tick generator
    for (int i = 0; i < num_ticks; i++) {
        q.push(feed.next_tick());           // at each itteration next_tick() outputs a tick that's pushed to queue
    }

    while (!q.empty()) {

        auto e = q.pop();       //e= event at front of queue

        if (!e.has_value()) break;      

        m.events_processed++;               //increment number of events processed

        if (auto* t = get_if<Tick>(&*e)) {             // If the event is a Tick, give me a pointer to it and...

            //RAPPEL: (struct: sequence number, price, created at)
            
            //we use a pointer *e to unwrap the optional and give you the actual variant

            // auto* because dk if TICK* or nothing


            last_price = t->price;      //t->price   ≡   (*t).price     (its a pointer)
            

            m.record_latency(t->created_at);            // access tick creation time stamp

            auto ord = strat.on_tick(*t);               //decides order or not

            if (ord.has_value()) q.push(*ord);          //if yes push to queue 


        } else if (auto* o = get_if<OrderRequest>(&*e)) {      //if order 

            //RAPPEL:  (struct: ID, side, quantity, time stamp)


            m.record_latency(o->created_at);

            if (risk.allow(*o)) {   //if safe

                auto fill = exec.execute(*o, last_price); 

                if (fill.has_value()) q.push(*fill);            //push fill to queue
            }


        } else if (auto* f = get_if<Fill>(&*e)) {
            m.record_latency(f->created_at);
            port.on_fill(*f);           //update portfolio
        }
    }
    m.finish();             //update end value for total time 
    port.print_final();         //print final portfolio
    m.print_summary();          //print metrics after having complete latency vector
    return 0;
}   