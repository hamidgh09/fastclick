// -*- c-basic-offset: 4 -*-
#ifndef CLICK_ELEMENTT_HH
#define CLICK_ELEMENTT_HH
#include "eclasst.hh"
#include "landmarkt.hh"

struct ElementT {
    
    int flags;

    ElementT();
    ElementT(const String &name, ElementClassT *type, const String &config, const LandmarkT &landmark = LandmarkT::empty_landmark());
    ~ElementT();

    RouterT *router() const		{ return _owner; }
    int eindex() const			{ return _eindex; }
    
    bool live() const			{ return _type; }
    bool dead() const			{ return !_type; }
    void full_kill();
    inline void simple_kill();

    const String &name() const		{ return _name; }
    const char *name_c_str() const	{ return _name.c_str(); }
    bool anonymous() const		{ return _name && _name[0] == ';'; }
    
    ElementClassT *type() const		{ return _type; }
    ElementClassT *resolved_type() const;
    ElementClassT *resolved_type(VariableEnvironment &ve, ErrorHandler *errh = 0) const;
    String type_name() const		{ return _type->name(); }
    String printable_type_name() const	{ return _type->printable_name(); }
    const char *type_name_c_str() const	{ return _type->printable_name_c_str(); }
    void set_type(ElementClassT *);
    inline bool compound() const;
    inline bool resolved_compound() const;
    RouterT *resolved_router() const;

    const String &config() const	{ return _configuration; }
    const String &configuration() const	{ return _configuration; }
    inline void set_configuration(const String &s);

    const LandmarkT &landmarkt() const	{ return _landmark; }
    String landmark() const		{ return _landmark.str(); }
    String decorated_landmark() const	{ return _landmark.decorated_str(); }
    void set_landmark(const LandmarkT &lm) { _landmark = lm; }

    inline bool tunnel() const;
    inline bool tunnel_connected() const;
    ElementT *tunnel_input() const	{ return _tunnel_input; }
    ElementT *tunnel_output() const	{ return _tunnel_output; }

    int nports(bool isoutput) const	{ return isoutput ? _noutputs : _ninputs; }
    int ninputs() const			{ return _ninputs; }
    int noutputs() const		{ return _noutputs; }
    
    inline String declaration() const;
    inline String reverse_declaration() const;

    void *user_data() const		{ return _user_data; }
    void set_user_data(void *v)		{ _user_data = v; }
    void set_user_data(intptr_t v)	{ _user_data = (void *)v; }

    static bool name_ok(const String &, bool allow_anon_names = false);
    static void redeclaration_error(ErrorHandler *, const char *type, String name, const String &landmark, const String &old_landmark);
    
  private:

    int _eindex;
    String _name;
    ElementClassT *_type;
    mutable ElementClassT *_resolved_type;
    enum { resolved_type_expand = 1, resolved_type_error = 2 };
    mutable int _resolved_type_status;
    String _configuration;
    LandmarkT _landmark;
    int _ninputs;
    int _noutputs;
    ElementT *_tunnel_input;
    ElementT *_tunnel_output;
    RouterT *_owner;
    void *_user_data;

    ElementT(const ElementT &);
    ElementT &operator=(const ElementT &);

    inline void unresolve_type() {
	if (_resolved_type) {
	    _resolved_type->unuse();
	    _resolved_type = 0;
	}
    }
    inline void set_ninputs(int n) {
	unresolve_type();
	_ninputs = n;
    }
    inline void set_noutputs(int n) {
	unresolve_type();
	_noutputs = n;
    }

    friend class RouterT;
    
};

struct PortT {
  
    ElementT *element;
    int port;

    PortT()				: element(0), port(-1) { }
    PortT(ElementT *e, int p)		: element(e), port(p) { }

    static const PortT null_port;

    typedef bool (PortT::*unspecified_bool_type)() const;

    operator unspecified_bool_type() const {
	return element != 0 ? &PortT::live : 0;
    }
    
    bool live() const			{ return element != 0; }
    bool dead() const			{ return element == 0; }
    RouterT *router() const		{ return (element ? element->router() : 0); }

    int eindex() const			{ return (element ? element->eindex() : -1); }
    
    int index_in(const Vector<PortT> &, int start = 0) const;
    int force_index_in(Vector<PortT> &, int start = 0) const;

    String unparse_input() const;
    String unparse_output() const;
    
    static void sort(Vector<PortT> &);

};

class ConnectionT { public:

    inline ConnectionT();
    ConnectionT(const PortT &, const PortT &, const LandmarkT & = LandmarkT::empty_landmark());
    ConnectionT(const PortT &, const PortT &, const LandmarkT &, int, int);

    typedef PortT::unspecified_bool_type unspecified_bool_type;
    inline operator unspecified_bool_type() const;

    bool live() const			{ return _from.live(); }
    bool dead() const			{ return _from.dead(); }

    RouterT *router() const		{ return _to.router(); }
    const PortT &from() const		{ return _from; }
    const PortT &to() const		{ return _to; }
    ElementT *from_element() const	{ return _from.element; }
    int from_eindex() const		{ return _from.eindex(); }
    int from_port() const		{ return _from.port; }
    ElementT *to_element() const	{ return _to.element; }
    int to_eindex() const		{ return _to.eindex(); }
    int to_port() const			{ return _to.port; }
    String landmark() const		{ return _landmark.str(); }
    String decorated_landmark() const	{ return _landmark.decorated_str(); }
    const LandmarkT &landmarkt() const	{ return _landmark; }

    int next_from() const		{ return _next_from; }
    int next_to() const			{ return _next_to; }
    
    String unparse() const;

  private:

    PortT _from;
    PortT _to;
    LandmarkT _landmark;
    int _next_from;
    int _next_to;

    friend class RouterT;
    
};


inline void
ElementT::simple_kill()
{
    if (_type)
	_type->unuse();
    _type = 0;
    unresolve_type();
}

inline void
ElementT::set_type(ElementClassT *t)
{
    assert(t);
    t->use();
    if (_type)
	_type->unuse();
    unresolve_type();
    _type = t;
}

inline bool
ElementT::compound() const
{
    return _type && _type->cast_router();
}

inline bool
ElementT::resolved_compound() const
{
    ElementClassT *t = resolved_type();
    return t && t->cast_router();
}

inline RouterT *
ElementT::resolved_router() const
{
    if (ElementClassT *t = resolved_type())
	return t->cast_router();
    else
	return 0;
}

inline void
ElementT::set_configuration(const String &configuration)
{
    unresolve_type();
    _configuration = configuration;
}

inline String
ElementT::declaration() const
{
    assert(_type);
    return _name + " :: " + _type->printable_name();
}

inline String
ElementT::reverse_declaration() const
{
    assert(_type);
    return _type->printable_name() + " " + _name;
}

inline bool
ElementT::tunnel() const
{
    return _type == ElementClassT::tunnel_type();
}

inline bool
ElementT::tunnel_connected() const
{
    return _tunnel_input || _tunnel_output;
}

inline bool
operator==(const PortT &h1, const PortT &h2)
{
    return h1.element == h2.element && h1.port == h2.port;
}

inline bool
operator!=(const PortT &h1, const PortT &h2)
{
    return h1.element != h2.element || h1.port != h2.port;
}

inline bool
operator<(const PortT &h1, const PortT &h2)
{
    return h1.eindex() < h2.eindex() || (h1.element == h2.element && h1.port < h2.port);
}

inline bool
operator>(const PortT &h1, const PortT &h2)
{
    return h1.eindex() > h2.eindex() || (h1.element == h2.element && h1.port > h2.port);
}

inline bool
operator<=(const PortT &h1, const PortT &h2)
{
    return h1.eindex() < h2.eindex() || (h1.element == h2.element && h1.port <= h2.port);
}

inline bool
operator>=(const PortT &h1, const PortT &h2)
{
    return h1.eindex() > h2.eindex() || (h1.element == h2.element && h1.port >= h2.port);
}

inline
ConnectionT::ConnectionT()
    : _from(), _to(), _landmark(), _next_from(-1), _next_to(-1)
{
}

inline
ConnectionT::operator unspecified_bool_type() const
{
    return (unspecified_bool_type) _from;
}

#endif
