using UnityEngine;
using System;
using System.Collections;
using Leap;
//using System.IO.Ports;
using System.Threading;
using System.IO;
using System.Collections.Generic;

public class Paper : MonoBehaviour {
	public GameObject guitext;
	private TextMesh gText;

	Controller controller = new Controller();	

	private Frame currentFrame;

	int move;

	List<int> previous = new List<int>();

	//SerialPort com4Port = new SerialPort("COM4",9600,Parity.None, 8, StopBits.One);

	// Use this for initialization
	void Start () {

		previous.Add (-1);

		gText = guitext.GetComponent<TextMesh> ();

		if(controller.IsConnected)
		{
			currentFrame = controller.Frame();
		}

		//if(!com4Port.IsOpen)
		//{
			//com4Port.Open();
		//}

	}
	
	// Update is called once per frame
	void Update () {

		move = CheckHands ();
		Debug.Log (CheckHands ());

		//if(CheckHands ())
		//{
			//previous.Add(CheckHands ());

			int counter = 0;
			switch(CheckHands ())
			{
			case 1: counter = 3;
				break;
			case 2: counter = 1;
				break;
			case 3: counter = 2;
				break;

			}
			using(StreamWriter w = new StreamWriter("rsp_in.txt"))
			{
				Debug.Log(counter);
				w.Write(counter);
			}
		//}

		//SendHandInfo (move);
	}

	int CheckHands()
	{
		int ct = 0;
		if(GameObject.Find("Fingers") != null)
		{
			if(GameObject.Find("THUMB") != null)
			{
				if(GameObject.Find("THUMB").activeSelf)
				{
					ct++;
				}
			}
			if(GameObject.Find("PINKY") != null)
			{
				if(GameObject.Find("PINKY").activeSelf)
				{
					ct++;
				}
			}
			if(GameObject.Find("MIDDLE") != null)
			{
				if(GameObject.Find("MIDDLE").activeSelf)
				{
					ct++;
				}
			}
			if(GameObject.Find("RING") != null)
			{
				if(GameObject.Find("RING").activeSelf)
				{
					ct++;
				}
			}
			if(GameObject.Find("INDEX") != null)
			{
				if(GameObject.Find("INDEX").activeSelf)
				{
					ct++;
				}
			}
		}
	
		if(ct == 0)
		{
			gText.text = "ROCK";

			return 1;
		}
		else if(ct == 2)
		{
			gText.text = "SCISSORS";

			return 2;
		}
		else if(ct == 5)
		{
			gText.text = "PAPER";

			return 3;
		}
		else
		{
			return -1;
		}

		return 0;
	}
	/*
	void SendHandInfo(int playerMove)
	{
		if(com4Port.IsOpen)
		{
			Debug.Log("sending " + playerMove);
			com4Port.Write(BitConverter.GetBytes(playerMove), 0, 4);
		}
	}
	*/
}
