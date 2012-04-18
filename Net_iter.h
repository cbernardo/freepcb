// definitions of Iterators for nets, connections, segments and vertices
#pragma once

class CIterator_cnet : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	CNetList const * m_NetList;

	POSITION m_CurrentPos;
	cnet * m_pCurrentNet;

public:
	explicit CIterator_cnet( CNetList const * netlist );
	~CIterator_cnet() {}

	cnet *GetFirst();
	cnet *GetNext();

public:
	static void OnRemove( cnet const * net );
};

class CIterator_cconnect : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cnet * m_net;

	int m_CurrentPos;
	cconnect * m_pCurrentConnection;

public:
	explicit CIterator_cconnect( cnet * net );
	~CIterator_cconnect();

	cconnect *GetFirst();
	cconnect *GetNext();

public:
	void OnRemove( int ic );
	void OnRemove( cconnect * con );
	int GetIndex(){ return m_CurrentPos; };
};

class CIterator_cseg : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cconnect * m_cconnect;

	int m_CurrentPos, m_PreviousPos, m_NextPos;
	cseg * m_pCurrentSegment;

public:
	explicit CIterator_cseg( cconnect * con );
	~CIterator_cseg() {}

	cseg *GetFirst();
	cseg *GetNext();

public:
	void OnRemove( int is );
	void OnRemove( cseg * seg );
	int GetIndex(){ return m_CurrentPos; };
};

class CIterator_cvertex : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cconnect * m_cconnect;

	int m_CurrentPos;
	cvertex * m_pCurrentVertex;

public:
	explicit CIterator_cvertex( cconnect * con );
	~CIterator_cvertex() {}

	cvertex *GetFirst();
	cvertex *GetNext();

public:
	void OnRemove( int iv );
	void OnRemove( cvertex * vtx );
	int GetIndex(){ return m_CurrentPos; };
};

class CIterator_cpin : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cnet * m_net;

	int m_CurrentPos;
	cpin * m_pCurrentConnection;

public:
	explicit CIterator_cpin( cnet * net );
	~CIterator_cpin();

	cpin *GetFirst();
	cpin *GetNext();

public:
	void OnRemove( int ip );
	void OnRemove( cpin * pin );
	int GetIndex(){ return m_CurrentPos; };
};

class CIterator_carea : protected CDLinkList
{
	// List of all active iterators
	static CDLinkList m_LIST_Iterator;

	cnet * m_net;

	int m_CurrentPos;
	carea * m_pCurrentConnection;

public:
	explicit CIterator_carea( cnet * net );
	~CIterator_carea();

	carea *GetFirst();
	carea *GetNext();

public:
	void OnRemove( int ic );
	void OnRemove( carea * a );
	int GetIndex(){ return m_CurrentPos; };
};

