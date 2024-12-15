#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/applications-module.h"


using namespace ns3;

int main (int argc, char *argv[])
{
  // сonfigure log conponent for debugging (optional)
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // set up node container for AP and STA
  NodeContainer wifiStaNode;
  wifiStaNode.Create(1); // Create 1 STA node
  NodeContainer wifiApNode;
  wifiApNode.Create(1); // Create 1 AP node

  // define Wi-Fi Channel and PHY settings
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper();
  phy.SetChannel(channel.Create());

  // setup 802.11ax Wi-Fi Standard
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ax); // Set Wi-Fi 6 (802.11ax)

  WifiMacHelper mac;
  Ssid ssid = Ssid("ns3-80211ax");

  // configure STA MAC
  mac.SetType(
    "ns3::StaWifiMac",
    "Ssid", SsidValue(ssid),
    "ActiveProbing", BooleanValue(false)
  );

  NetDeviceContainer staDevice;
  staDevice = wifi.Install(phy, mac, wifiStaNode);

  // configure AP MAC
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  
  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // set mobility model for AP and STA
  MobilityHelper mobility;

  // AP position (fixed)
  mobility.SetPositionAllocator(
    "ns3::GridPositionAllocator",
    "MinX", DoubleValue(0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (5.0),
    "DeltaY", DoubleValue (10.0),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst")
  );
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNode);

  // STA position (fixed)
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiStaNode);

  // install internet stack
  InternetStackHelper stack;
  stack.Install(wifiApNode);
  stack.Install(wifiStaNode);

  // asign IP adresses
  Ipv4AddressHelper address;
  address.SetBase("192.168.1.0", "255.255.255.0");

  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign(staDevice);

  Ipv4InterfaceContainer apInterface;
  apInterface =  address.Assign(apDevice);

  // setup UDP Echo Server on AP
  UdpEchoServerHelper echoServer(9);
  ApplicationContainer serverApp = echoServer.Install(wifiApNode.Get(0));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  // setup UDP Echo Client on STA
  UdpEchoClientHelper echoClient(apInterface.GetAddress(0),9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));


  ApplicationContainer clientApp = echoClient.Install(wifiStaNode.Get(0));
  clientApp.Start(Seconds(2.0));
  clientApp.Stop(Seconds(10.0));

  // enable PCAP tracing for packets
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.EnablePcap("wifi-80211ax", apDevice.Get(0));

  // run simulation
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}