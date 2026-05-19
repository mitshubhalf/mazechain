#ifndef RPC_AUTH_H
#define RPC_AUTH_H

// MazeChain — JSON-RPC Authentication & Admin Console
// HTTP Basic Auth for RPC endpoints
// JSON-RPC 2.0 over HTTP (POST /rpc)
// Admin console: runtime config changes

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <functional>

namespace RPC {

    // ── Basic Auth ─────────────────────────────────────────────────────────────
    static std::string g_rpcUser     = "mazechain";
    static std::string g_rpcPassword = "changeme";
    static bool        g_rpcEnabled  = true;

    // Base64 encode (minimal implementation for Basic Auth)
    inline std::string base64Encode(const std::string& input) {
        static const char* chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (uint8_t c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(chars[((val << 8) >> (valb+8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string base64Decode(const std::string& input) {
        static const int T[256] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,63,
            52,53,54,55,56,57,58,59,60,61,0,0,0,0,0,0,
            0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,0,0,0,0,0,
            0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,0,0,0,0,0
        };
        std::string out;
        int val=0, valb=-8;
        for (uint8_t c : input) {
            if (c > 127 || T[c] == 0) { valb -= 6; continue; }
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    inline bool authenticate(const std::string& authHeader) {
        if (!g_rpcEnabled) return true; // open if disabled
        // "Basic <base64(user:pass)>"
        if (authHeader.substr(0, 6) != "Basic ") return false;
        std::string decoded = base64Decode(authHeader.substr(6));
        size_t colon = decoded.find(':');
        if (colon == std::string::npos) return false;
        return decoded.substr(0, colon) == g_rpcUser &&
               decoded.substr(colon+1)  == g_rpcPassword;
    }

    inline std::string expectedAuth() {
        return "Basic " + base64Encode(g_rpcUser + ":" + g_rpcPassword);
    }

    inline void setCredentials(const std::string& user, const std::string& pass) {
        g_rpcUser = user; g_rpcPassword = pass;
    }

    // ── JSON-RPC 2.0 Types ─────────────────────────────────────────────────────
    struct RPCRequest {
        std::string id;
        std::string method;
        std::vector<std::string> params;
        bool        valid = false;
    };

    struct RPCResponse {
        std::string id;
        std::string result;
        std::string error;
        bool        isError = false;

        std::string toJSON() const {
            std::ostringstream ss;
            ss << "{\"jsonrpc\":\"2.0\",\"id\":" << id;
            if (isError)
                ss << ",\"error\":{\"code\":-32000,\"message\":\"" << error << "\"}";
            else
                ss << ",\"result\":" << result;
            ss << "}";
            return ss.str();
        }
    };

    // Simple JSON-RPC parser (no heavy deps)
    inline RPCRequest parseRequest(const std::string& body) {
        RPCRequest req;
        auto find = [&](const std::string& key) -> std::string {
            size_t pos = body.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = body.find(':', pos); if (pos == std::string::npos) return "";
            pos = body.find_first_not_of(" \t", pos+1);
            if (pos == std::string::npos) return "";
            if (body[pos] == '"') {
                size_t end = body.find('"', pos+1);
                return body.substr(pos+1, end-pos-1);
            }
            size_t end = body.find_first_of(",}", pos);
            return body.substr(pos, end-pos);
        };
        req.id     = find("id");
        req.method = find("method");
        req.valid  = !req.method.empty();
        // Extract params array (simplified: comma-separated strings)
        size_t pa = body.find("\"params\"");
        if (pa != std::string::npos) {
            size_t ob = body.find('[', pa);
            size_t cb = body.find(']', ob);
            if (ob != std::string::npos && cb != std::string::npos) {
                std::string arr = body.substr(ob+1, cb-ob-1);
                std::istringstream ss(arr);
                std::string tok;
                while (std::getline(ss, tok, ',')) {
                    size_t s = tok.find('"');
                    size_t e = tok.rfind('"');
                    if (s != std::string::npos && e > s)
                        req.params.push_back(tok.substr(s+1, e-s-1));
                    else {
                        std::string t = tok;
                        t.erase(0, t.find_first_not_of(" \t"));
                        t.erase(t.find_last_not_of(" \t")+1);
                        if (!t.empty()) req.params.push_back(t);
                    }
                }
            }
        }
        return req;
    }

    // RPC method registry
    using RPCHandler = std::function<RPCResponse(const RPCRequest&)>;
    static std::map<std::string, RPCHandler> g_methods;

    inline void registerMethod(const std::string& name, RPCHandler handler) {
        g_methods[name] = handler;
    }

    inline RPCResponse dispatch(const RPCRequest& req) {
        if (!req.valid) return {req.id, "", "Invalid request", true};
        auto it = g_methods.find(req.method);
        if (it == g_methods.end())
            return {req.id, "", "Method not found: " + req.method, true};
        try { return it->second(req); }
        catch (const std::exception& e) { return {req.id, "", e.what(), true}; }
    }

    inline std::string handleRequest(const std::string& body) {
        auto req  = parseRequest(body);
        auto resp = dispatch(req);
        return resp.toJSON();
    }

} // namespace RPC

#endif
