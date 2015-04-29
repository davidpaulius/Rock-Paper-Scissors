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

	public Transform mB3;
	public Transform rB3;

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

	}
	
	// Update is called once per frame
	void Update () {

		if(GameObject.Find("ThickRigidHand(Clone)") != null)
		{
			mB3 = GameObject.Find("ThickRigidHand(Clone)").transform.FindChild("middle").FindChild("bone3");
			rB3 = GameObject.Find("ThickRigidHand(Clone)").transform.FindChild("ring").FindChild("bone3");
			Debug.Log("M= " + mB3.rotation.eulerAngles.z);
			Debug.Log("R= " + rB3.rotation.eulerAngles.z);
			if( (mB3.rotation.eulerAngles.z > 140 && mB3.rotation.eulerAngles.z < 240)
			   && (rB3.rotation.eulerAngles.z > 150 && rB3.rotation.eulerAngles.z < 250) )
			{
				gText.GetComponent<TextMesh>().text = "Rock";
			}
			else if( (mB3.rotation.eulerAngles.z > 295 && mB3.rotation.eulerAngles.z < 360)
			   && (rB3.rotation.eulerAngles.z > 295 && rB3.rotation.eulerAngles.z < 360) )
			{
				gText.GetComponent<TextMesh>().text = "Paper";
			}
			else if( ( (mB3.rotation.eulerAngles.z > 295 && mB3.rotation.eulerAngles.z < 360) //336
			        && (rB3.rotation.eulerAngles.z > 190 && rB3.rotation.eulerAngles.z < 250) ) //208
			        ||
			        ( (mB3.rotation.eulerAngles.z > 180 && mB3.rotation.eulerAngles.z < 330) //336
			        && (rB3.rotation.eulerAngles.z > 190 && rB3.rotation.eulerAngles.z < 280) ) //208
			        )
			{
				gText.GetComponent<TextMesh>().text = "Scissors";
			}
			else
			{
				gText.GetComponent<TextMesh>().text = "RPS";

			}

		}
		else
		{
			gText.GetComponent<TextMesh>().text = "RPS";
		}


		using(StreamWriter w = new StreamWriter("rsp_in.txt"))
		{
			if(gText.GetComponent<TextMesh>().text == "ROCK")
			{
				w.WriteLine(1);
			}

			switch(gText.GetComponent<TextMesh>().text)
			{

			case "Rock": w.WriteLine(1); Debug.Log(1);
				break;

			case "Paper" : w.WriteLine(2); Debug.Log(2);
				break;

			case "Scissors": w.WriteLine(3); Debug.Log(3);
				break;

			default: w.WriteLine(-1); Debug.Log(-1);
				break;

			}
		}

	}

}
