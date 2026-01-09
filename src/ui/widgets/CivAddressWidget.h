#ifndef CIVADDRESSWIDGET_H
#define CIVADDRESSWIDGET_H

#include <QWidget>
#include <QRadioButton>
#include <QLineEdit>
#include <QButtonGroup>

namespace TR4QT {

/**
 * Reusable widget for CI-V address selection
 *
 * Provides two modes:
 * - Default (0x00): Radio responds to broadcast commands
 * - Custom: User-specified hex address (e.g., 0x94 for IC-7300)
 *
 * Used by both RadioConfigDialog and PreferencesDialog to eliminate
 * code duplication.
 */
class CivAddressWidget : public QWidget {
    Q_OBJECT

public:
    explicit CivAddressWidget(QWidget* parent = nullptr);
    ~CivAddressWidget() override = default;

    /**
     * Set the CI-V address value
     * @param address CI-V address (0 = default/auto, other = custom hex value)
     */
    void setCivAddress(int address);

    /**
     * Get the current CI-V address value
     * @return CI-V address (0 = default, other = custom hex value)
     */
    int getCivAddress() const;

    /**
     * Auto-configure CI-V address for known Icom radio models
     * @param hamlibModelId Hamlib model ID (3000-3999 for Icom)
     */
    void autoConfigureForRadio(int hamlibModelId);

    // Override to provide proper size hint for QFormLayout
    QSize sizeHint() const override;

private slots:
    void onCivAddressModeChanged();

private:
    void setupUI();

    QButtonGroup* m_civButtonGroup;
    QRadioButton* m_civDefaultRadio;
    QRadioButton* m_civCustomRadio;
    QLineEdit* m_civAddressEdit;
};

} // namespace TR4QT

#endif // CIVADDRESSWIDGET_H
