#ifndef __TICKABLE_PINS_H__
#define __TICKABLE_PINS_H__
#include "StandardLib\Object.h"
#include "StandardLib\Array.h"
#include "Port.h"


class CTickablePins : public CObject
{
    //SNAPSHOT extends Snapshot,
    //PINS extends Pins<SNAPSHOT, PINS, ? extends IntegratedCircuit<SNAPSHOT, PINS>>,
    //INTEGRATED_CIRCUIT extends IntegratedCircuit<SNAPSHOT, PINS>>

protected:
    INTEGRATED_CIRCUIT integratedCircuit;

    Ptr<CTickables>   mpcTickables;
    CArray<CPort>     macPorts;

    uint64            muiTickCount;

public:
    Ptr<CTickablePins> Init(Ptr<CTickables> pcTickables)
    {
        mpcTickables = pcTickables;
        mpcTickables.Add(Ptr<CTickablePins>(this));
        macPorts.Init();
        muiTickCount = 0LL;
    }

    void AddPort(Ptr<CPort> pcPort)
    {
        macPorts.Add(pcPort);
    }

    final void StartPropagation(void)
    {
        snapshot = integratedCircuit.createSnapshot();
        getIntegratedCircuit().startTick();
    }

    void Propagate(void)
    {
        undoPropagation();
        getIntegratedCircuit().tick();
    }


    List<TraceValue> GetTraceValues(void)
    {
        List<TraceValue> traceValues = new ArrayList<>();
        for (Port port : ports)
        {
            port.addTraceValues(traceValues);
        }
        return traceValues;
    }

    void UpdateConnections(void)
    {
        for (Port port : ports)
        {
            port.updateConnection();
        }
    }

    void ResetConnections(void)
    {
        for (Port port : ports)
        {
            port.resetConnections();
        }
    }

    String GetName(void)
    {
        return getIntegratedCircuit().getName();
    }

    String GetDescription(void)
    {
        String name = getName();
        if (StringUtil.isEmptyOrNull(name))
        {
            return getType();
        }
        else
        {
            return getType() + " \"" + name + "\"";
        }
    }

    protected BusValue GetBusValue(Ptr<COmniport> pcOmniport)
    {
        TraceValue value = omniport.read();
        if (value.isError())
        {
            return BusValue.error();
        }
        else if (value.isNotConnected())
        {
            return BusValue.notConnected();
        }
        else if (value.isUnsettled())
        {
            return BusValue.unknown();
        }
        else
        {
            return new BusValue(omniport.getPinsAsBoolAfterRead());
        }
    }

    protected PinValue GetPinValue(Uniport uniport)
    {
        TraceValue value = uniport.read();
        return getPinValue(value);
    }

    protected PinValue getPinValue(TraceValue value)
    {
        if (value.isError())
        {
            return PinValue.Error;
        }
        else if (value.isNotConnected())
        {
            return PinValue.NotConnected;
        }
        else if (value.isUnsettled())
        {
            return PinValue.Unknown;
        }
        else
        {
            return value.isHigh() ? PinValue.High : PinValue.Low;
        }
    }

    char ToDebugString(CChars* pszDest)
    {
        StringBuilder stringBuilder = new StringBuilder();
        String description = " " + getDescription() + " ";
        stringBuilder.append(" ").append(StringUtil.centerJustify(description, 48, "-")).append(" \n");
        for (Port port : ports)
        {
            String portTransmissionState = StringUtil.rightJustify(port.getPortTransmissionStateAsString(), 24, " ");
            String portValues = port.getTraceValuesAsString();
            String connectionValues = port.getConnectionValuesAsString();
            if (portValues.equals(connectionValues))
            {
                stringBuilder.append(portTransmissionState).append(": P&W").append(portValues);
            }
            else
            {
                stringBuilder.append(portTransmissionState).append(":   P").append(portValues).append(" W");
                stringBuilder.append(connectionValues);
            }
            List<Trace> connections = port.getConnections();
            Set<Port> updatingPorts = get_DEBUG_UpdatingPorts(connections);
            if (updatingPorts.size() > 1)
            {
                stringBuilder.append(" (Multiple updaters)");
            }
            else if (updatingPorts.size() == 1)
            {
                stringBuilder.append(" (").append(updatingPorts.iterator().next().getDescription()).append(")");
            }

            stringBuilder.append("\n");
        }
        return stringBuilder.toString();
    }

    private Set<Port> get_DEBUG_UpdatingPorts(List<Trace> connections)
    {
        Set<Port> updatingPorts = new LinkedHashSet<>();
        for (Trace connection : connections)
        {
            Port updatingPort = connection.get_DEBUG_lastPortThatUpdated();
            if (updatingPort != null)
            {
                updatingPorts.add(updatingPort);
            }
        }
        return updatingPorts;
    }

    INTEGRATED_CIRCUIT GetIntegratedCircuit(void)
    {
        return integratedCircuit;
    }

    void SetIntegratedCircuit(INTEGRATED_CIRCUIT integratedCircuit)
    {
        this.integratedCircuit = integratedCircuit;
    }

    String GetType(void)
    {
        return integratedCircuit.getType();
    }

    uint64 GetTickCount(void)
    {
        return muiTickCount;
    }
}


#endif // !__TICKABLE_PINS_H__

