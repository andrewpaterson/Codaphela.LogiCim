#ifndef __PORT_H__
#define __PORT_H__
#include "StandardLib\Object.h"
#include "TransmissionState.h"
#include "TickablePins.h"


class CPort : public CObject
{
CONSTRUCTABLE(CPort);
DESTRUCTABLE(CPort);

protected:
    Ptr<CTickablePins>  mpcTickable;
    CChars              mszName;
    ETransmissionState  meState;

public:
    void Init(Ptr<CTickablePins> pcTickable, char* szName)
    {
        this.tickable = tickable;
        this.name = name;
        this.state = NotSet;
        tickable.addPort(this);
    }

    String getName()
    {
        return name;
    }

    void resetConnections()
    {
        state = NotSet;
    }

    virtual void addTraceValues(List<TraceValue> traceValues);

    virtual void updateConnection();

    virtual TraceValue read();

    static TraceValue readStates(Collection< ? extends Port> ports)
    {
        boolean high = false;
        boolean low = false;
        boolean error = false;
        boolean unsettled = false;
        boolean connected = false;

        for (Port port : ports)
        {
            TraceValue value = port.read();
            if (value.isConnected())
            {
                connected = true;
            }

            if (value.isError())
            {
                error = true;
            }
            else if (value.isUnsettled())
            {
                unsettled = true;
            }
            else if (value.isHigh())
            {
                high = true;
            }
            else if (value.isLow())
            {
                low = true;
            }
        }

        return translatePortValue(high, low, error, unsettled, connected);
    }

    static TraceValue translatePortValue(boolean high, boolean low, boolean error, boolean unsettled, boolean connected)
    {
        if (connected)
        {
            if (error)
            {
                return Error;
            }
            else if (unsettled)
            {
                return Unsettled;
            }
            else if (high && low)
            {
                return HighAndLow;
            }
            else if (high)
            {
                return High;
            }
            else if (low)
            {
                return Low;
            }
        }
        return NotConnected;
    }

    String getPortTransmissionStateAsString()
    {
        String portStateString = "  ";
        if (state.isInput())
        {
            portStateString = "<-";
        }
        else if (state.isOutput())
        {
            portStateString = "->";
        }
        else if (state.isNotSet())
        {
            portStateString = "..";
        }
        else if (state.isImpedance())
        {
            portStateString = "xx";
        }
        return getName() + "[" + portStateString + "]";
    }

    String getDescription()
    {
        return getTickable().getDescription() + "." + getName();
    }

    TickablePins getTickable()
    {
        return tickable;
    }

    virtual List<Trace> getConnections();

    virtual String getTraceValuesAsString();

    virtual String getWireValuesAsString();

    virtual String getConnectionValuesAsString();
};


#endif // !__PORT_H__

