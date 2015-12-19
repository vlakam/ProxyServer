//
// Created by kamenev on 05.12.15.
//

#ifndef POLL_EVENT_PROXY_SERVER_H
#define POLL_EVENT_PROXY_SERVER_H



#include <memory>
#include "connection.h"
#include <cstddef>
#include "address.h"
#include "HTTP.h"
#include "acceptor.h"
#include "events.h"
#include "outstring.h"
#include <map>
#include <regex>
#include <queue>
#include <boost/signals2.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <mutex>

class proxy_server
{
    struct inbound;
    struct outbound;
    struct inbound
    {
        inbound(proxy_server *parent);
        void handleread();
        void handlewrite();
        void sendBadRequest();
        bool resolveFinished(std::string domain,ipv4_address);
        void sendDomainForResolve(std::string);
    private:
        friend struct outbound;
        proxy_server *parent;
        connection socket;
        std::shared_ptr<request> requ;
        boost::signals2::connection resolverConnection;
        std::shared_ptr<outbound> assigned;
        //std::pair<std::queue<std::string>, std::size_t> output;
        std::queue<outstring> output;

    };
    struct outbound
    {
        outbound(io::io_service&,ipv4_endpoint,inbound *);
        void handlewrite();
        void handleread();
        void handleContentRead();
    private:
        friend struct inbound;
        ipv4_endpoint remote;
        connection socket;
        inbound* assigned;
        std::shared_ptr<response> resp;
        std::string host;
        std::string URI;
    };
public:
    proxy_server(io::io_service &ep, ipv4_endpoint const &local_endpoint);
    ~proxy_server();
    ipv4_endpoint local_endpoint() const;
    typedef std::queue<std::pair<std::string, ipv4_address>> Resolvequeue;
    struct FirstFound
    {
        typedef bool result_type;
        template <typename InputIterator> result_type operator()(InputIterator aFirstObserver, InputIterator aLastObserver) const {
            result_type val = false;
            for (; aFirstObserver != aLastObserver && !val; ++aFirstObserver)  {
                val = *aFirstObserver;
            }
            return val;
        }
    };
    boost::signals2::signal<bool (std::string,ipv4_address), FirstFound> resolver;
    boost::lockfree::queue<std::string*,boost::lockfree::capacity<30>> domains;
    boost::condition_variable newTask;
    boost::mutex resolveMutex;
    std::mutex distributeMutex;
    Resolvequeue resolverFinished;
    events resolveEvent;
    io::io_service *batya;
    boost::thread_group resolvers;
    bool destroyThreads = false;
private:
    void on_new_connection();
    friend struct inbound;
    friend struct outbound;

    acceptor ss;
    std::map<std::string,ipv4_endpoint> dnsCache;
    std::map<inbound *, std::unique_ptr<inbound>> connections;
};



#endif //POLL_EVENT_PROXY_SERVER_H