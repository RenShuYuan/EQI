/***************************************************************************
                             qgsprojectproperties.h
       Set various project properties (inherits qgsprojectpropertiesbase)
                              -------------------
  begin                : May 18, 2004
  copyright            : (C) 2004 by Gary E.Sherman
  email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgsoptionsdialogbase.h"
#include "ui_qgsprojectpropertiesbase.h"
#include "qgis.h"
#include "qgisgui.h"
#include "qgscontexthelp.h"

class QgsMapCanvas;
class QgsLayerTreeGroup;

/** Dialog to set project level properties

  @note actual state is stored in QgsProject singleton instance

 */
class APP_EXPORT QgsProjectProperties : public QgsOptionsDialogBase, private Ui::QgsProjectPropertiesBase
{
    Q_OBJECT

  public:
    //! ¹¹Ôìº¯Êý
    QgsProjectProperties( QgsMapCanvas* mapCanvas, QWidget *parent = nullptr, Qt::WindowFlags fl = QgisGui::ModalDialogFlags );

    //! Îö¹¹º¯Êý
    ~QgsProjectProperties();

    /** »ñÈ¡µ±Ç°Ñ¡¶¨µÄµØÍ¼µ¥Î»
     */
    QGis::UnitType mapUnits() const;

    /*!
     * ÉèÖÃµØÍ¼µ¥Î»
     */
    void setMapUnits( QGis::UnitType );

    /*!
       Ã¿¸öÏîÄ¿¶¼ÓÐÒ»¸ö±êÌâ
     */
    QString title() const;
    void title( QString const & title );

    /** ±íÊ¾¸ÃÍ¶Ó°¿ª¹Ø½ÓÍ¨ */
    bool isProjected();

  public slots:
    /*!
     * Ó¦ÓÃ°´Å¥(slot)
     */
    void apply();

    /*!
     * ¶Ô»°¿òÏÔÊ¾Í¶Ó°Ñ¡Ïî¿¨´ò¿ªÊ±(slot)
     */
    void showProjectionsTab();

    /** ÈÃÓÃ»§µÄ±ÈÀýÌí¼Óµ½×éºÏ¿òµÄ¹æÄ££¬¶ø²»ÊÇÄÇÐ©È«ÇòÏîÄ¿Ê¹ÓÃ±ÈÀýÁÐ±í */
    void on_pbnAddScale_clicked();

    /** ÈÃÓÃ»§ÔÚ¹æÄ£×éºÏ¿ò£¬¶ø²»ÊÇÄÇÐ©È«ÇòÏîÄ¿Ê¹ÓÃ±ÈÀýÁÐ±íÖÐÉ¾³ý¹æÄ£ */
    void on_pbnRemoveScale_clicked();

    /** ÈÃÓÃ»§´ÓÎÄ¼þÖÐ¼ÓÔØ±ÈÀý */
    void on_pbnImportScales_clicked();

    /** ÈÃÓÃ»§½«±ÈÀýµ¼³öµ½ÎÄ¼þ */
    void on_pbnExportScales_clicked();

    /** A scale in the list of project scales changed ÔÚÏîÄ¿³ß¶ÈµÄÁÐ±íµÄ±ÈÀý³ß±ä»¯*/
    void scaleItemChanged( QListWidgetItem* changedScaleItem );

    /*!
     * Slot to show the context help for this dialog
     */
    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }

    void on_cbxProjectionEnabled_toggled( bool onFlyEnabled );

    /*!
      * Èç¹ûÓÃ»§¸ü¸ÄÁËCSS£¬ÉèÖÃÏàÓ¦µÄµØÍ¼µ¥Î»
      */
    void srIdUpdated();

    /* ¸ù¾ÝËùÑ¡ÔñµÄÐÂµÄË÷Òý¸üÐÂµÄComboBoxÁíÍâÉèÖÃÐÂÑ¡¶¨ÍÖÇò¡£*/
    void updateEllipsoidUI( int newIndex );

    //! ÉèÖÃÕýÈ·µÄÍÖÇò²âÁ¿£¨ÉèÖÃ£©
    void projectionSelectorInitialized();

  signals:
    //! ÓÃÀ´Í¨ÖªÊó±êÏÔÊ¾¾«¶È¿ÉÄÜÒÑ¾­¸Ä±ä
    void displayPrecisionChanged();

    //! ÓÃÓÚÍ¨ÖªÕìÌýÏîÄ¿±ÈÀýÁÐ±í¿ÉÄÜÒÑ¾­¸Ä±ä
    void scalesChanged( const QStringList &scales = QStringList() );

    //! ÈÃ»­²¼ÖªµÀË¢ÐÂ
    void refresh();

  private:

    //! ×ø±êÏÔÊ¾¸ñÊ½
    enum CoordinateFormat
    {
      DecimalDegrees, /*!< Ê®½øÖÆ¶È */
      DegreesMinutes, /*!< ¶È, ·Ö */
      DegreesMinutesSeconds, /*!< ¶È£¬·Ö£¬Ãë*/
      MapUnits, /*! ÏÔÊ¾µØÍ¼×ø±êµ¥Î» */
    };

    QgsMapCanvas* mMapCanvas;

    /*!
     * ±£´æ·Ç»ù±¾¶Ô»°×´Ì¬
     */
    void saveState();

    /*!
     * ¹¦ÄÜ»Ö¸´·Ç»ù±¾¶Ô»°×´Ì¬
     */
    void restoreState();

    long mProjectSrsId;
    long mLayerSrsId;

    // ÁÐ±íÖÐµÄËùÓÐÍÖÇòÌå£¬Ò²Ã»ÓÐºÍ×Ô¶¨Òå
    struct EllipsoidDefs
    {
      QString acronym;
      QString description;
      double semiMajor;
      double semiMinor;
    };
    QList<EllipsoidDefs> mEllipsoidList;
    int mEllipsoidIndex;

	//! ´ÓSQLITE3 dbÖÐÌî³äÍÖÇòÌåÁÐ±í
    void populateEllipsoidList();

    //! ´´½¨Ò»¸öÐÂµÄ±ÈÀý³ßÏî£¬²¢½«ÆäÌí¼Óµ½±ÈÀýÁÐ±í
    QListWidgetItem* addScaleToScaleList( const QString &newScale );

	//! Ìí¼ÓÒ»¸ö±ÈÀý³ßÏîµ½±ÈÀý³ßÁÐ±í
    void addScaleToScaleList( QListWidgetItem* newItem );

    static const char * GEO_NONE_DESC;

    void updateGuiForMapUnits( QGis::UnitType units );
};
