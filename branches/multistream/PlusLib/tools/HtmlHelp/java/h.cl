����  -6 Code 
SourceFile ConstantValue 
Exceptions HHCtrl  java/applet/Applet  J 
m_winStyle 
 		          I 	m_command  	   
LTreeView; m_tview  	   ()Ljava/awt/Dimension; size   java/awt/Component 
   width   java/awt/Dimension 	    height " 	   # (IIII)V reshape & %
  ' ()V repaint * )
  + Ljava/lang/Thread; m_workerThread . -	  / stop 1 ) java/lang/Thread 3
 4 2 LRelatedDialog; m_dlgRelated 7 6	  8 hide : )
  ; Ljava/util/Vector; 
q_commands > =	  ? java/lang/Integer A (I)V <init> D C
 B E (Ljava/lang/Object;)V 
addElement H G java/util/Vector J
 K I 	q_objects M =	  N 	notifyAll P ) java/lang/Object R
 S Q (Ljava/awt/LayoutManager;)V 	setLayout V U java/awt/Container X
 Y W TreeView [ (Ljava/applet/Applet;)V D ]
 \ ^ (II)V D `
   a (Ljava/awt/Dimension;)V setSize d c
 \ e *(Ljava/awt/Component;)Ljava/awt/Component; add h g
 Y i addControls k )
 \ l 
m_sizeMode n 	  o Ljava/awt/Color; 	m_bgcolor r q	  s (Ljava/awt/Color;)V setBackground v u
 \ w Ljava/awt/Image; 	m_bgImage z y	  { (Ljava/awt/Image;)V setBackgroundImage ~ }
 \  m_itemHeight � 	  � setItemHeight � C
 \ � m_style � 	  � setStyle � C
 \ � m_exWinStyle � 		  � (JJ)V 	setStyles � �
 \ � m_redrawMode � 	  � setRedrawMode � C
 \ � m_clickMode � 	  � setClickMode � C
 \ � m_autoExpandLevel � 	  � setAutoExpandLevel � C
 \ � Ljava/awt/Font; m_font � �	  � (Ljava/awt/Font;)V setFont � �
  � %(Ljava/awt/Event;Ljava/lang/Object;)Z gotFocus � �
 \ � 	lostFocus � �
 \ � wait � )
 S � java/lang/InterruptedException � ()I  �
 K � (I)Ljava/lang/Object; 	elementAt � �
 K � intValue � �
 B � removeElementAt � C
 K � Z fLoaded � �	  � initList � )
  � Element � loadElement � G
  � java/lang/String � Ljava/lang/String; m_targetFrame � �	  � '(Ljava/lang/String;Ljava/lang/String;)V loadURL � �
  � showRelated � )
  � m_itemParams � =	  �   � (Ljava/lang/String;)I 	compareTo � �
 � � java/lang/StringBuffer � D )
 � � cnt.load � &(Ljava/lang/String;)Ljava/lang/String; 	getString � �
  � ,(Ljava/lang/String;)Ljava/lang/StringBuffer; append � �
 � � :  � ()Ljava/lang/String; toString � �
 � � (Ljava/lang/String;)V 
showStatus � �
  � java/net/URL ()Ljava/net/URL; getDocumentBase
  #(Ljava/net/URL;Ljava/lang/String;)V D
 (Ljava/net/URL;)Z loadFromHHC

 \ getFrame �
 \ java/net/MalformedURLException cnt.load.err   black q java/awt/Color	 m_fontColor q	  java/awt/Font 	Helvetica! (Ljava/lang/String;II)V D#
 $ ,& indexOf( �
 �) (II)Ljava/lang/String; 	substring,+
 �- length/ �
 �0 (I)Ljava/lang/String;,2
 �3 parseInt5 �
 B6 java/lang/Exception8 trim: �
 �; toUpperCase= �
 �> (Ljava/lang/String;I)I5@
 BA
 E BOLDD ITALICF java/awt/ButtonH m_buttonJ �	 K D �
IM Ljava/awt/Button; 
btnRelatedPO	 Q java/awt/BorderLayoutS
T � CenterV <(Ljava/lang/String;Ljava/awt/Component;)Ljava/awt/Component; hX
 YY m_relatedParams[ =	 \ RelatedDialog^ rel.dlgcaption` ()Ljava/awt/Frame; getParentFramecb
 d ?(Ljava/lang/String;Ljava/lang/String;LHHCtrl;Ljava/awt/Frame;)V Df
_g Dialogi ()Z CreateControlslk
_m Ljava/awt/Dimension; m_Sizepo	 q (Z)V 	setRedrawts
 \u 
doValidatew )
 \x grayz q	{ setColor} u java/awt/Graphics
�~ drawLine� %
�� white� q	� 	lightGray� q	� 
IndexPanel� (LHHCtrl;)V D�
�� LIndexPanel; m_index��	 �
� w
�  "(Ljava/awt/Font;Ljava/awt/Color;)V ��
��
�m validate� )
 Y� [Ljava/lang/String;� Command� String� button� Button� Font� Flags� 
Background� BackgroundImage� Background Image� Frame� 
Properties� Property file� Ljava/awt/List; lstItems��	_� getSelectedIndex� � java/awt/List�
�� 
m_itemList� =	_� ;� cnt.load.status� (Ljava/lang/String;)Z equalsIgnoreCase��
 �� Ljava/util/Properties; m_locStrings��	 � idx.load.status2� getProperty� � java/util/Properties�
�� cnt.load.status2� (Ljava/lang/String;I)V sync��
 \� java.vendor� java/lang/System�
�� 	Microsoft� #�
M ()Ljava/applet/AppletContext; getAppletContext��
 � (Ljava/net/URL;)V showDocument�� java/applet/AppletContext������ 
err.badurl� (Ljava/awt/Graphics;)V paint��
 � ()Ljava/lang/Thread; currentThread��
 4� (Ljava/lang/Thread;)V setCallingThread��
_� (Ljava/util/Vector;)V setItems 
_ show )
_ 	getStatus �
_ suspend
 )
 4 loadRelated )
  (ILjava/lang/Object;)V addJob
  [J m_flags	      � 5�������� 0x 
startsWith�
 � 0X! (Ljava/lang/String;I)J 	parseLong$# java/lang/Long&
'%       5 (Ljava/lang/Runnable;)V D+
 4, start. )
 4/ requestFocus1 )
 2 *HTML Help Java Applet Version 4.72.7346 
4 BCopyright (C) 1996-1997 Microsoft Corporation. All rights reserved6 m_url8 �	 �9 m_text; �	 �< Ljava/lang/Object; target?> java/awt/EventA	B@ ()Ljava/awt/Container; 	getParentED
 F java/awt/FrameH
�
� idx.load.errL runJobN )
 O
  2 getParameterR �
 S RELATED TOPICSU INDEXW
 K � ItemZ (I)Ljava/lang/StringBuffer; �\
 �] ((Ljava/lang/String;I)Ljava/lang/Integer; valueOf`_
 Ba  (Ljava/net/URL;)Ljava/awt/Image; getImagedc
 e #(Ljava/lang/String;)Ljava/awt/Font; 	parseFonthg
 i 
parseFlagsk �
 l (III)V Dn
o
� � java/io/StringBufferInputStreamr m_engStringst �	 u
sM (Ljava/io/InputStream;)V loadyx
�z (Ljava/util/Properties;)V D|
�} getCodeBase
 � java/io/BufferedInputStream� ()Ljava/io/InputStream; 
openStream��
� (Ljava/io/InputStream;I)V D�
�� ()Ljava/lang/Runtime; 
getRuntime�� java/lang/Runtime�
�� java/io/DataInputStream� Dx
�� ,(Ljava/io/InputStream;)Ljava/io/InputStream; getLocalizedInputStream��
�� err.propload� initContents� )
 � 	initIndex� )
 � initRelated� )
 � (Ljava/awt/Event;)Z handleEvent��
 � 	m_related� =	 �� m_target� �	 ��
  � STYLE_WIN95� 	 \� maxWidth� 	 � 	maxHeight� 	 � minWidth� 	 � 	minHeight� 	 � PARAM_command� �	 � PARAM_button� �	 � Item1� PARAM_item1� �	 � PARAM_background� �	 � PARAM_backgroundimage� �	 � PARAM_properties� �	 � 
PARAM_font� �	 � PARAM_flags� �	 � PARAM_frame� �	 � SizeMode� PARAM_sizemode� �	 � &err.propload=Error loading properties
� err.badurl=Invalid URL:
� cnt.load=Loading
� $cnt.load.status=Loading contents...
�  cnt.load.elementname=Loading...
� /cnt.load.success=Contents loaded successfully.
� "cnt.load.status2=Loading contents
� cnt.load.err=Error loading
� cnt.merge.err=Error merging
� !cnt.merge.elementname=Loading...
� )cnt.merge.errelement=Cannot load section
� /cnt.merge.success=Section loaded successfully.
� idx.load.err=Error loading
� +idx.load.success=Index loaded successfully
� !idx.load.status=Loading index...
� idx.load.status2=Loading index
�  idx.load.elementname=Loading...
  idx.display=Display
 rel.dlgwidth=294
 rel.dlgheight=238
 rel.btnwidth=50
 rel.dlgcaption=Topics Found

 rel.display=Display
 rel.cancel=Cancel
 -rel.label=Click a topic, then click Display.
 (Ljava/awt/Event;I)Z keyDown
 \ java/lang/Runnable HHCtrl.java run CMD_CONTENTS     	CMD_INDEX    CMD_RELATED    fSized 	m_autoKey WCMD_LOADLIST WCMD_SHOWDOC WCMD_SHOWURL    
WCMD_CLICK    m_initState  �   2 getAppletInfo getParameterInfo ()[[Ljava/lang/String; init update action setWinStyle (J)V syncURL click HHClick     5                      po    � �    z y    q    � 	    
 	        �          �     �     �     �     n    ��   !     r q   PO    7 6    > =    M =    . -   ��   "       #       $      % &      '      J �    � �   ( �    �     � =   [ =    � �   �      ) �      ) �      * �      * � �     � � �     � � �     � � �     � � �     � � �     � � �     � � �     � � �     � � �     � t �      12     a     U*� *�  	�� **� � #*� *� � !d*� � $d� (*� ,�*� *� � !*� � $� (*� ,�      1 )     6     **� 0� *� 0� 5*� 0*� � *� 9� 
*� 9� <�     "     (     *� @� BY� F� L*� O,� L*� T�     � )         �*� Z*� \Y*� _� *� �  Y*� � !*� � $� b� f**� � jW*� � m*�  	�� (*� p� !*� *� � !d*� � $d� (� !*� p� *� *� � !*� � $� (*� *� t� x*� *� |� �*� *� �� �*� *� �� �*� *� *� �� �*� *� �� �*� *� �� �*� *� �� �*� *� �� �*� ,�      � �          
*� +,� ��      � �          
*� +,� ��     "N )     �     �� *� �� W*� @� ����*� @� �� B� �<*� O� �M*� @� �*� O� ��    C            (   1   >*� �*� ͱ*,� Ϸ ұ*,� �*� ط ܱ*� ߱�    
 �     � )         �<*� � �� �*� �� �� �� � �*� �Y� �*� � ��� �*� �� �� Զ �� �� �Y*�*� �� �� Է	M*� ,�<*� �� � >**� �� ا 0W*� �Y� �*� � �� �*� �� �� Զ �� �� � /*� �Y� �*� � �� �*� �� �� Զ �� �� *� ,*� � ,�  I � �   hg    �  
  {>6+� *��� Y"�%�":66+'�*6�  � +�.M� +M,�1� ,:� F+`�4L+'�*6� +�.M� +M,�1� ,�76� 6� W6� +`�4L+'�*6� [+`�4L+'�*6� +�.�<�?M� +�<�?M,�1� #,�B6	*�Y	�C�� W*��� Z+`�4L+'�*6� +�.�<�?M� +�<�?M6,�1� ,E�*� �,G�*� �� Y�%�  � � �9 �	9   � )     o     c*�IY*�L�N�R*�TY�U� Z*W*�R�ZW**� �]*�_Y*a� �*� �**�e�h� 9*� Yj�%� �*� 9�nW�     ��    �    �**� �r*� p� �*� � !� �*� � $� �*� � �*� �v*�  	�� I*� *� � !d*� � $d� (*� �v*� �  Y*� � !d*� � $d� b� f� >*� *� � !*� � $� (*� �v*� �  Y*� � !*� � $� b� f*� �y*�  	�� �*� � �+�|��+*�r� $��+*�r� !��+���+*�r� !d��+*�r� $d��+����+*�r� $d*�r� !*�r� $d��+*�r� !d*�r� !d*�r� $��+����+*�r� $d*�r� !d*�r� $d��+*�r� !d*�r� $d���     � )     �     �*	� *�TY�U� Z*��Y*����*��*� t��*��*� |��*��*� �*���*W*���ZW*����W*��*�_Y*a� �*� �**�e�h� 9*� Yj�%� �*� 9�nW�     ,-     �     ���Y� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSY� �Y�SY�SY�SSL+�      )     X     L*� 9����� �*� 9��*� 9����� �� �M,� �,Ķ*>� �,`�4L*+*� ط ܱ      � �     O     C+ƶʙ *� � *��жհ+׶ʙ *� � *��жհ*��+�հ     3 �          *� � *� +�۱      � �     �     �ݸ�N-�*� 8+�*� -� �Y� �++�*`�.� �++�*�4� �� �L�Y+��:,� � *���� �*��,�� �W�Y*�+�	:,� � *���� �*��,�� �W*� �Y� �*� � �� �+� �� �� �  G r s t � �   /�          *� p� *+���       � )     �     �*�]� �� T��N*� 9-��*� 9*�]�*� 9�*� 9�	� -�*� 9�	<�                  *�*�]� �� .*�]� �� �N-Ķ*6� �-`�4M*,��     k �    j    F+� !*� P*�P*�P�+'�*>� L� +�.M� +M,�1� 4,� � ,"� �  *�,�4�(P� W*� P� \+`�4L+'�*>� +�.M� +M,�1� 4,� � ,"� �  *�,�4�(P� W*�)P� \+`�4L+'�*>� +�.M� +M,�1� 4,� � ,"� �  *�,�4�(P� W*�P**�/� **�/� ��  Z j m9 � � �9$'9   . )     3     '**� �r*� 0� *� 4Y*�-� 0*� 0�0*�3�     + �     #     � �Y� �5� �7� �� ��     0 �     �     �*� � N,� ϴ:� � 2*� �Y� �*� � ��� �,� ϴ:� �� �� *,�� *,� ϴ=� �*� � ,� �� 	*,��*� � +�C*�R� *���     cb     2     &*�GL� +�GL+� 
+�I���+�I� +�I��     5 )          *� � 
*��     4 )          *� � 
*��      )     �     �*� � *� ʚ *� �*� �*� � w*� ʚ p*���Y*�*� �� �� Է	�JW*���K� � **���K� �*� ʧ 0W*� �Y� �*M� � �� �*� �� �� Զ �� �� *�P���W*�Q���  & c f � � � �   . )    �    �*��TL+� $+�?V� � *� +X�ʙ *� *��TL+� *+�L*� KY�Y� �=� *� �+� L*� �Y� �[� ���^� ��TYL���*� � 
*��� t*� � 
*��� t*��TL+� "+�b� �>� *�Y�C� t� W*��TL+�  **�Y*�+�	�f� |� 	W*� |***��T�j� �**��T�m� ��p*��TL+� *+� ػ�Y�qN-�sY*�v�w�{� W*��Y-�~��*��TL+� N�Y*��+�	:��Y��  ��:����Y������:*���{� W**�� � *� �   (                #*���*���*����  � � �9 � � �9%479P��9   ��          *+���      � G     j     ^+� ϴ�� ;+� ϴ:� � G*+� ϴ:+� ϴ�� � 
*� ا 
+� ϴ�� ܱ*+� ϴ��]� 4Y*� 9�-M,�0�      D )    v    j*��*��*� �* � *� �*��� �*� �*� �*��� t*�L*� �*� �*ܵ�*ܵ�*2��*2��*���*���*ŵ�*���*���*���*���*���*���*ܵ�*� �Y� �� �� �� �� �� �� ��� �� �� �� ��� ��� ��� ��� ��� ��� �� �� �� �� �	� �� �� �� �� �� ��v*��*� KY�Y� @*� KY�Y� O�               *� � *� +���         